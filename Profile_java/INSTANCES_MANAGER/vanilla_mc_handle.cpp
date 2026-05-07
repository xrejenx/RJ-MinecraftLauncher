#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

// External logger interface
void LogLauncherEvent(const QString &message);

/**
 * vanilla_mc_handle.cpp - Handles fetching Minecraft JAR download URLs.
 */

QVariantMap FetchVanillaMetadata(const QString &versionId) {
    LogLauncherEvent("Fetching JAR URL for version: " + versionId);
    QNetworkAccessManager manager;
    QEventLoop loop;

    // Step 1: Get the main version manifest
    QNetworkRequest manifestRequest{QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")};
    QNetworkReply *manifestReply = manager.get(manifestRequest);
    QObject::connect(manifestReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(); // Blocks until reply is finished

    if (manifestReply->error() != QNetworkReply::NoError) {
        LogLauncherEvent("Failed to fetch version manifest: " + manifestReply->errorString());
        manifestReply->deleteLater();
        return QVariantMap();
    }

    QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestReply->readAll());
    manifestReply->deleteLater();

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
                    QString hashPrefix = hash.left(2);
                    QString assetUrl = QString("https://resources.download.minecraft.net/%1/%2").arg(hashPrefix, hash);
                    QString assetPath = QString("objects/%1/%2").arg(hashPrefix, hash);

                    QJsonObject assetEntry;
                    assetEntry["url"] = assetUrl;
                    assetEntry["path"] = assetPath;
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
    QStringList libUrls;
    QJsonArray libs = root["libraries"].toArray();
    for (const auto &l : libs) {
        QJsonObject libObj = l.toObject();
        if (libObj.contains("downloads")) {
            QJsonObject artifact = libObj["downloads"].toObject()["artifact"].toObject();
            QString url = artifact["url"].toString();
            if (!url.isEmpty()) {
                libUrls << url;
            }
        }
    }
    result["libraries"] = libUrls;
    result["mainClass"] = root["mainClass"].toString();

    return result;
}