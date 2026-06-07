#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QProcess>
#include "Core.h"

void DownloadAndInstallUpdate(const QString &url, int version) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager.get(request);
    
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QString zipPath = MinecraftLauncher::getRJLDataPath() + QString("LauncherUpdater/update-%1.zip").arg(version);
        QFile file(zipPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            
            QString tool = QCoreApplication::applicationDirPath() + "/Tools/MadeChanges.exe";
            if (QFile::exists(tool)) {
                QProcess::startDetached(tool, {"--update", zipPath});
                QCoreApplication::quit();
            }
        }
    }
    reply->deleteLater();
}