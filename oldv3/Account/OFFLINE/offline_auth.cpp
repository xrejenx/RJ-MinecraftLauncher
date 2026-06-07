#include <QJsonObject>
#include <QUuid>
#include <QString>

QJsonObject CreateOfflineAccountData(const QString &username) {
    QJsonObject acc;
    acc["Username"] = username;
    // Generate a simple random UUID for offline use
    acc["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces).replace("-", "");
    acc["accessToken"] = "0";
    acc["userType"] = "offline";
    return acc;
}