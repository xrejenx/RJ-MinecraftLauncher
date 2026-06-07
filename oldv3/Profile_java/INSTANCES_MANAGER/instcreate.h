#pragma once

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QWidget>

class InstCreate : public QObject {
    Q_OBJECT
public:
    explicit InstCreate(QObject *parent = nullptr);

    // Function to finalize instance creation, display results, and clean up
    void finalizeInstance(const QString &instanceName, const QString &versionId,
                          const QString &javaVersionPath, bool mcDownloadSuccess, bool lwjglDownloadSuccess,
                          QWidget *parentWidget);

signals:
    void instanceCreationFinished(); // Signal to notify main window to refresh instance list
};
