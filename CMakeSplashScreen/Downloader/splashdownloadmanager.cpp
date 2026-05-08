#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

void DownloadAndInstallUpdate(const QString &url, int version) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QString zipPath = QString("update_%1.zip").arg(version);
        QFile file(zipPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            
            // Extraction and self-replacement logic
            QProcess p;
#ifdef Q_OS_WIN
            p.start("powershell", {"-Command", QString("Expand-Archive -Path %1 -DestinationPath . -Force").arg(zipPath)});
#else
            p.start("unzip", {"-o", zipPath});
#endif
            p.waitForFinished();
            
            // Relaunch
            QProcess::startDetached(QCoreApplication::applicationFilePath());
            QCoreApplication::quit();
            exit(0);
        }
    }
}