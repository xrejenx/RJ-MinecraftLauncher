#include <QString>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include "Core.h"

extern QJsonObject CreateOfflineAccountData(const QString &username);

bool SaveOfflineAccount(const QString &username) {
    QString dataPath = MinecraftLauncher::getRJLDataPath() + "userdata/OFFLINE";
    QDir().mkpath(dataPath);

    QFile file(dataPath + "/" + username + ".json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(CreateOfflineAccountData(username)).toJson());
        file.close();
        return true;
    }
    return false;
}