#pragma once

#include <QString>
#include <QJsonObject>

class SessionManager {
public:
    static SessionManager& instance();

    void loadSession();
    bool saveSession(const QByteArray &data);
    bool hasActiveSession() const;

    QString getUsername() const;
    QString getUuid() const;
    QString getAccessToken() const;
    QString getUserType() const;

private:
    SessionManager();
    QJsonObject m_session;
    QString getSessionPath() const;
};
