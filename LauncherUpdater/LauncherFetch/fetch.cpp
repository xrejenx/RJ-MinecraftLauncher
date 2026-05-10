#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QEventLoop>

// External dependencies
extern int GetBuildNumber();
void LogLauncherEvent(const QString &message);
extern QString GetBuildTypePrefix(); // From version.cpp

QString GetBuildTypePrefixShort() {
#ifdef LAUNCHER_BUILD_TYPE_SHORT
    return QString(LAUNCHER_BUILD_TYPE_SHORT);
#else
    return "r"; // Fallback to release
#endif
}

extern QString GetUpdateRepositoryUrl();

struct ReleaseAsset {
    QString name;
    QString url;
    qint64 size;
};

struct ReleaseInfo {
    int buildNumber;
    QString tagName;
    QString description;
    bool isPreRelease;
    QList<ReleaseAsset> assets;
};

struct UpdateInfo {
    QList<ReleaseInfo> allReleases;
};

/**
 * fetch.cpp - Fetches SHA-1 or SHA-256 hash strings from a remote URL.
 */
QString FetchRemoteHash(const QString &url) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) return QString(reply->readAll()).trimmed();
    return QString();
}

/**
 * CheckForUpdates - The new modern implementation using LauncherUpdater/LauncherFetch.
 */
UpdateInfo CheckForUpdates() {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(GetUpdateRepositoryUrl() + "/releases");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "RJML-Modern-Updater");
    
    QNetworkReply *reply = manager.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    UpdateInfo info;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray releases = QJsonDocument::fromJson(reply->readAll()).array();
        
        for (const QJsonValue &v : releases) {
            QJsonObject rel = v.toObject();
            QString tagName = rel["tag_name"].toString();
            int remoteBuild = tagName.remove(QRegularExpression("[^\\d]")).toInt();

            ReleaseInfo rInfo;
            rInfo.buildNumber = remoteBuild;
            rInfo.tagName = tagName;
            rInfo.description = rel["body"].toString();
            rInfo.isPreRelease = rel["prerelease"].toBool();

            QJsonArray assetsJson = rel["assets"].toArray();
            for (const auto &a : assetsJson) {
                QJsonObject assetObj = a.toObject();
                rInfo.assets.append({assetObj["name"].toString(), assetObj["browser_download_url"].toString(), assetObj["size"].toVariant().toLongLong()});
            }
            info.allReleases.append(rInfo);
        }
    } else {
        LogLauncherEvent("Update Check Failed: " + reply->errorString());
    }
    return info;
}

/**
 * CheckForInstallerUpdates - Specifically for the Installer's self-update check.
 * Follows the RJInstallerWin64-<type><build>.exe naming and branch matching logic.
 */
UpdateInfo CheckForInstallerUpdates() {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QUrl url(GetUpdateRepositoryUrl() + "/releases");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "RJML-Installer-Updater");
    
    QNetworkReply *reply = manager.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    UpdateInfo info;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray releases = QJsonDocument::fromJson(reply->readAll()).array();
        
        // Logic: u user only sees u installers, b sees b, r sees r.
        QString typeKey = GetBuildTypePrefixShort().toLower();

        for (const QJsonValue &v : releases) {
            QJsonObject rel = v.toObject();
            ReleaseInfo rInfo;
            rInfo.tagName = rel["tag_name"].toString();
            rInfo.buildNumber = rInfo.tagName.remove(QRegularExpression("[^\\d]")).toInt();

            QJsonArray assetsJson = rel["assets"].toArray();
            for (const auto &a : assetsJson) {
                QJsonObject assetObj = a.toObject();
                QString assetName = assetObj["name"].toString();

                // Filter for: RJInstallerWin64-<type><number>.exe
                QRegularExpression re("RJInstallerWin64-([ubr])(\\d+)\\.exe");
                QRegularExpressionMatch match = re.match(assetName);
                
                if (match.hasMatch()) {
                    QString foundType = match.captured(1);
                    if (foundType == typeKey) {
                        rInfo.assets.append({assetName, assetObj["browser_download_url"].toString(), assetObj["size"].toVariant().toLongLong()});
                    }
                }
            }
            
            if (!rInfo.assets.isEmpty()) {
                info.allReleases.append(rInfo);
            }
        }
    }
    return info;
}