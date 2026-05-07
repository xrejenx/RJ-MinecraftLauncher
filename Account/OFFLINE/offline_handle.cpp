#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

/**
 * offline_handle.cpp - Main handler for offline account storage and processing
 */

// Forward declaration for logic in offline_auth.cpp
QJsonObject CreateOfflineAccountData(const QString &username);

bool SaveOfflineAccount(const QString &username) {
    if (username.isEmpty()) return false;

    QJsonObject acc = CreateOfflineAccountData(username);
    
    fs::create_directories("userdata/OFFLINE");
    std::string path = "userdata/OFFLINE/" + username.toStdString() + ".json";
    
    std::ofstream file(path);
    if (file.is_open()) {
        file << QJsonDocument(acc).toJson().toStdString();
        file.close();
        return true;
    }
    return false;
}