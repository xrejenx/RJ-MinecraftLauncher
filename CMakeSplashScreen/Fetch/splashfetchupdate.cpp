#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QEventLoop>

extern int GetBuildNumber();

struct UpdateInfo {
    bool releaseAvailable = false;
    int releaseBuild = 0;
    QString releaseUrl;

    bool preReleaseAvailable = false;
    int preReleaseBuild = 0;
    QString preReleaseUrl;
};

UpdateInfo CheckForUpdates() {
    QNetworkAccessManager manager;
    QEventLoop loop;
    
    // Read GitHub repository path from embedded api.txt
    QString repoPath;
    QFile apiFile(":/config/CMakeSplashScreen/api.txt");
    if (apiFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&apiFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("api =")) {
                repoPath = line.section('=', 1).trimmed();
                break;
            }
        }
        apiFile.close();
    }

    // Query the list of releases instead of just /latest to find pre-releases
    QUrl url(QString("https://api.github.com/repos/%1/releases").arg(repoPath));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "RJML-Updater");
    
    QNetworkReply *reply = manager.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    UpdateInfo info;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray releases = QJsonDocument::fromJson(reply->readAll()).array();
        int local = GetBuildNumber();
        
        for (const QJsonValue &v : releases) {
            QJsonObject rel = v.toObject();
            int remote = rel["tag_name"].toString().toInt();
            bool isPre = rel["prerelease"].toBool();

            if (remote <= local) continue;

            QString downloadUrl;
            QJsonArray assets = rel["assets"].toArray();
            for (const auto &a : assets) {
                QString name = a.toObject()["name"].toString().toLower();
#ifdef Q_OS_WIN
                if (name.contains("win")) downloadUrl = a.toObject()["browser_download_url"].toString();
#else
                if (name.contains("linux")) downloadUrl = a.toObject()["browser_download_url"].toString();
#endif
            }

            if (isPre && !info.preReleaseAvailable && remote > info.preReleaseBuild) {
                info.preReleaseAvailable = true;
                info.preReleaseBuild = remote;
                info.preReleaseUrl = downloadUrl;
            } else if (!isPre && !info.releaseAvailable && remote > info.releaseBuild) {
                info.releaseAvailable = true;
                info.releaseBuild = remote;
                info.releaseUrl = downloadUrl;
            }
        }
    }
    return info;
}