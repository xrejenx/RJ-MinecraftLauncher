#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>

// Forward declaration
int GetBuildNumber();

struct UpdateResult {
    bool available;
    int version;
    QString downloadUrl;
};

UpdateResult FetchLatestUpdate() {
    QNetworkAccessManager manager;
    QEventLoop loop;
    
    // GitHub API for latest release
    QUrl url("https://api.github.com/repos/xrejenx/RJMinecraftLauncher/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "RJLauncher-Updater");
    
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    UpdateResult res = {false, 0, ""};
    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        QString tagName = json["tag_name"].toString(); // e.g., "31"
        int remoteBuild = tagName.toInt();
        int localBuild = GetBuildNumber();
        
        if (remoteBuild > localBuild) {
            res.available = true;
            res.version = remoteBuild;
            // Logic to find linux/windows specific asset URL in json["assets"]
            res.downloadUrl = json["assets"].toArray().at(0).toObject()["browser_download_url"].toString();
        }
    }
    reply->deleteLater();
    return res;
}