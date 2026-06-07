#include "download.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QUrl>

bool DownloadFileToPath(const QString &url, const QString &destination) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager.get(request);
    
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
    return false;
}