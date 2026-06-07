#include "sessionman.h"
#include "Core.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>

SessionManager& SessionManager::instance() {
    static SessionManager inst;
    return inst;
}

SessionManager::SessionManager() {
    loadSession();
}

QString SessionManager::getSessionPath() const {
    return MinecraftLauncher::getRJLDataPath() + "userdata/LauncherSession.json";
}

void SessionManager::loadSession() {
    QFile file(getSessionPath());
    if (file.open(QIODevice::ReadOnly)) {
        m_session = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    } else {
        m_session = QJsonObject();
    }
}

bool SessionManager::saveSession(const QByteArray &data) {
    QDir().mkpath(MinecraftLauncher::getRJLDataPath() + "userdata");
    QFile file(getSessionPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        loadSession();
        return true;
    }
    return false;
}

bool SessionManager::hasActiveSession() const {
    return !m_session.isEmpty();
}

QString SessionManager::getUsername() const {
    if (m_session.contains("profile")) return m_session["profile"].toObject()["name"].toString();
    if (m_session.contains("username")) return m_session["username"].toString();
    if (m_session.contains("Username")) return m_session["Username"].toString();
    return "Guest";
}

QString SessionManager::getUuid() const {
    return m_session.value("uuid").toString("0");
}

QString SessionManager::getAccessToken() const {
    return m_session.value("accessToken").toString("0");
}

QString SessionManager::getUserType() const {
    // Detect based on content or use mojang default
    return m_session.value("userType").toString("mojang");
}
