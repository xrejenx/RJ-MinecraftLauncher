#pragma once

#include <QObject>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QJsonObject>

class Inst : public QObject {
    Q_OBJECT
public:
    explicit Inst(QObject *parent = nullptr);

    // Function to start Java download for a specific instance
    // Returns true on success, false on failure or cancellation
    bool downloadJavaForInstance(const QString &versionId, int javaMajorVersion, QWidget *parentWidget);

signals:
    void javaDownloadFinished(bool success, const QString &javaPath);

private:
    // Helper to find the best matching Java package
    QJsonObject findJavaPackage(int javaMajorVersion, const QString &os, const QString &arch);
};
