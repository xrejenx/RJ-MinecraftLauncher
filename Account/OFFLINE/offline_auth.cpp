#include <QJsonObject>
#include <QUuid>
#include <QString>

/**
 * offline_auth.cpp - Logic for generating offline account data structure
 */

QJsonObject CreateOfflineAccountData(const QString &username) {
    QJsonObject acc;
    acc["username"] = username;
    acc["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    acc["type"] = "offline";
    return acc;
}