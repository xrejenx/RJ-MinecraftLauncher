#ifndef INSTMC_H
#define INSTMC_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QWidget>
#include <QNetworkAccessManager> // Required for member declaration

class InstMC : public QObject {
    Q_OBJECT
public:
    explicit InstMC(QObject *parent = nullptr);
    bool downloadMinecraftAssets(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId, QWidget *parentWidget);
signals:
    void minecraftDownloadFinished(bool success);
private:
    QNetworkAccessManager *m_networkManager;
    int m_maxParallelDownloads;
    QString m_instanceNativesDir;
};

#endif // INSTMC_H