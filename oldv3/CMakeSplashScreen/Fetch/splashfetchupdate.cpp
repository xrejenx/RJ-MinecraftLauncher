#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QEventLoop>
#include <QRegularExpression>

extern int GetBuildNumber();
void LogLauncherEvent(const QString &message);
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

UpdateInfo CheckForUpdates() {
    QNetworkAccessManager manager;
    QEventLoop loop;

    // Construct the API URL using the centralized detector
    QUrl url(GetUpdateRepositoryUrl() + "/releases");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "RJML-Updater");
    
    QNetworkReply *reply = manager.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    UpdateInfo info;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray releases = QJsonDocument::fromJson(reply->readAll()).array();
        if (releases.isEmpty()) {
            LogLauncherEvent("Update Check: No releases found on GitHub.");
        }
        
        for (const QJsonValue &v : releases) {
            QJsonObject rel = v.toObject();
            
            QString tagName = rel["tag_name"].toString();
            int remote = tagName.remove(QRegularExpression("[^\\d]")).toInt();

            ReleaseInfo rInfo;
            rInfo.buildNumber = remote;
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
