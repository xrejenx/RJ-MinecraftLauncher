#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QTimer>

// External logger interface
void LogLauncherEvent(const QString &message);

/**
 * vanilla_mc_handle.cpp - Handles fetching Minecraft JAR download URLs.
 */

QVariantMap FetchVanillaMetadata(const QString &versionId) {
    LogLauncherEvent("Fetching JAR URL for version: " + versionId);
    QNetworkAccessManager manager;
    QEventLoop loop;

    QJsonDocument manifestDoc;
    bool success = false;

    // Retry logic: 5 times, 5s delay
    for (int i = 0; i < 5; ++i) {
        QNetworkRequest manifestRequest{QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")};
        QNetworkReply *manifestReply = manager.get(manifestRequest);
        QObject::connect(manifestReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (manifestReply->error() == QNetworkReply::NoError) {
            manifestDoc = QJsonDocument::fromJson(manifestReply->readAll());
            manifestReply->deleteLater();
            success = true;
            break;
        }
        
        LogLauncherEvent(QString("Manifest fetch failed: %1. Retry %2/5 in 5s...").arg(manifestReply->errorString()).arg(i+1));
        manifestReply->deleteLater();
        if (i < 4) { QEventLoop sleep; QTimer::singleShot(5000, &sleep, &QEventLoop::quit); sleep.exec(); }
    }

    if (!success) {
        return QVariantMap();
    }

    QJsonArray versions = manifestDoc.object()["versions"].toArray();
    QString versionUrl;
    for (const auto &v : versions) {
        QJsonObject obj = v.toObject();
        if (obj["id"].toString() == versionId) {
            versionUrl = obj["url"].toString();
            break;
        }
    }

    if (versionUrl.isEmpty()) {
        LogLauncherEvent("Version URL not found in manifest for ID: " + versionId);
        return QVariantMap();
    }

    // Step 2: Get the specific version's JSON
    QNetworkRequest versionRequest{QUrl(versionUrl)};
    QNetworkReply *versionReply = manager.get(versionRequest);
    QObject::connect(versionReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(); // Blocks until reply is finished

    if (versionReply->error() != QNetworkReply::NoError) {
        LogLauncherEvent("Failed to fetch version details for " + versionId + ": " + versionReply->errorString());
        versionReply->deleteLater();
        return QVariantMap();
    }

    QJsonDocument versionDoc = QJsonDocument::fromJson(versionReply->readAll());
    versionReply->deleteLater();

    QVariantMap result;
    QJsonObject root = versionDoc.object();
    
    // Client JAR
    result["clientUrl"] = root["downloads"].toObject()["client"].toObject()["url"].toString();

    // Get Required Java Version
    if (root.contains("javaVersion")) {
        result["javaMajor"] = root["javaVersion"].toObject()["majorVersion"].toInt();
    }

    // --- ASSET INDEX AND OBJECTS ---
    if (root.contains("assetIndex")) {
        QJsonObject assetIndexObj = root["assetIndex"].toObject();
        QString assetIndexId = assetIndexObj["id"].toString();
        QString assetIndexUrl = assetIndexObj["url"].toString();

        result["assetIndexId"] = assetIndexId;
        result["assetIndexUrl"] = assetIndexUrl;

        // Download the asset index JSON itself
        QNetworkRequest assetIndexJsonRequest{QUrl(assetIndexUrl)};
        QNetworkReply *assetIndexJsonReply = manager.get(assetIndexJsonRequest);
        QObject::connect(assetIndexJsonReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (assetIndexJsonReply->error() == QNetworkReply::NoError) {
            QJsonDocument assetIndexDoc = QJsonDocument::fromJson(assetIndexJsonReply->readAll());
            QJsonObject assetIndexRoot = assetIndexDoc.object();

            if (assetIndexRoot.contains("objects")) {
                QJsonObject objects = assetIndexRoot["objects"].toObject();
                QJsonArray assetObjectsArray;

                for (auto it = objects.begin(); it != objects.end(); ++it) {
                    QString hash = it.value().toObject()["hash"].toString();
                    QString legacyName = it.key();
                    QString hashPrefix = hash.left(2);
                    QString assetUrl = QString("https://resources.download.minecraft.net/%1/%2").arg(hashPrefix, hash);
                    QString assetPath = QString("objects/%1/%2").arg(hashPrefix, hash);

                    QJsonObject assetEntry;
                    assetEntry["url"] = assetUrl;
                    assetEntry["path"] = assetPath;
                    assetEntry["legacyPath"] = legacyName;
                    assetEntry["hash"] = hash;
                    assetObjectsArray.append(assetEntry);
                }
                result["assetObjects"] = assetObjectsArray;
            }
        } else {
            LogLauncherEvent("Failed to download asset index JSON for " + versionId + ": " + assetIndexJsonReply->errorString());
        }
        assetIndexJsonReply->deleteLater();
    }
    
    // Libraries (LWJGL, etc)
    QVariantList libEntries;
    QVariantList nativeEntries;
    QJsonArray libs = root["libraries"].toArray();
    QString detectedLwglVer;

    QString osName;
#ifdef Q_OS_WIN
    osName = "windows";
#elif defined(Q_OS_MAC)
    osName = "osx";
#else
    osName = "linux";
#endif

    for (const auto &l : libs) {
        QJsonObject libObj = l.toObject();
        
        QString libName = libObj["name"].toString();
        // Dynamic LWJGL version detection
        if (detectedLwglVer.isEmpty() && libName.contains("lwjgl")) {
            QStringList parts = libName.split(':');
            if (parts.size() >= 3) detectedLwglVer = parts[2];
            else if (parts.size() == 2) detectedLwglVer = parts[1];
        }

        // Rule check for OS compatibility
        bool allowed = true;
        if (libObj.contains("rules")) {
            allowed = false;
            QJsonArray rules = libObj["rules"].toArray();
            for (const auto &r : rules) {
                QJsonObject rule = r.toObject();
                QString action = rule["action"].toString();
                if (rule.contains("os")) {
                    if (rule["os"].toObject()["name"].toString() == osName) allowed = (action == "allow");
                } else {
                    allowed = (action == "allow");
                }
            }
        }
        if (!allowed) continue;

        if (libObj.contains("downloads")) {
            QJsonObject downloads = libObj["downloads"].toObject();
            if (downloads.contains("artifact")) {
                QJsonObject art = downloads["artifact"].toObject();
                QVariantMap entry;
                entry["url"] = art["url"].toString();
                entry["path"] = art["path"].toString();
                libEntries.append(entry);
            }
            // Handle natives classifiers for dynamic LWJGL versions
            if (libObj.contains("natives") && downloads.contains("classifiers")) {
                QString nativeKey = libObj["natives"].toObject()[osName].toString();
                if (downloads["classifiers"].toObject().contains(nativeKey)) {
                    QJsonObject nArt = downloads["classifiers"].toObject()[nativeKey].toObject();
                    QVariantMap entry;
                    entry["url"] = nArt["url"].toString();
                    entry["path"] = nArt["path"].toString();
                    nativeEntries.append(entry);
                }
            }
        }
    }

    // Strict LWJGL Version Mapping (The Requirements)
    // Minecraft 1.13+ uses LWJGL 3, everything else uses LWJGL 2.
    bool isModern = false;
    QStringList modernPrefixes = {"1.13", "1.14", "1.15", "1.16", "1.17", "1.18", "1.19", "1.20", "1.21"};
    for (const QString &p : modernPrefixes) {
        if (versionId.startsWith(p)) { isModern = true; break; }
    }

    // Fallback if detection failed
    if (detectedLwglVer.isEmpty()) detectedLwglVer = isModern ? "3.3.1" : "2.9.4-nightly-20150209";

    result["lwjglVersion"] = detectedLwglVer;
    result["lwjglMajor"] = isModern ? 3 : 2;
    LogLauncherEvent(QString("Requirement Check: MC %1 -> LWJGL %2 (v%3)").arg(versionId, detectedLwglVer, isModern ? "3" : "2"));

    result["libraries"] = libEntries;
    result["natives"] = nativeEntries;
    result["mainClass"] = root["mainClass"].toString();

    return result;
}
