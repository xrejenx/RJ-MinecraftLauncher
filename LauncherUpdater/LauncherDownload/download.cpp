#include "download.h" // Include its own header
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QEventLoop>
#include <QDebug>

/**
 * download.cpp - Core utility for downloading a single file synchronously.
 * This is used by modules that need a blocking download (e.g., for extraction libraries,
 * or when a simple, quick download is needed without complex progress UI).
 */
bool DownloadFileToPath(const QString &url, const QString &destination) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(destination);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            return true;
        }
    }
    qDebug() << "DownloadFileToPath failed for" << url << ":" << reply->errorString();
    reply->deleteLater();
    return false;
}