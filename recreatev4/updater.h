// updater.h
#pragma once

#include <QObject>
#include <QNetworkReply>
#include <QList>
#include <QFile>

class MainWindow;

struct RemoteAsset {
    QString name;
    QString url;
    int size = 0;
};

struct RemoteRelease {
    QString name;
    QString tag;
    QString body;
    QList<RemoteAsset> assets;
};

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(MainWindow *parentWindow);
    ~Updater() override;

    void initUpdater();
    void setupConnections();

    // Public helper to download 7zip directly (called from MainWindow)
    void downloadSevenZipDirectly();

signals:
    void progress(int done, int total);
    void logMessage(const QString &msg);
    void finished(bool ok);

private slots:
    // Slot used by QNetworkAccessManager::finished for releases fetch
    void onReleasesFetched(QNetworkReply *reply);

    // Internal asset download handlers (used by startInternalAssetDownload)
    void onAssetDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onAssetDownloadReadyRead();
    void onAssetDownloadedFinished();

private:
    // Parse and populate releases
    void parseReleases(const QByteArray &jsonData);
    void populateRemoteList();

    // External tool flow (launches MadeChanges.exe)
    void startAssetDownload(const RemoteAsset &asset);

    // Internal direct-download flow (download raw files into Tools/7zip)
    void startInternalAssetDownload(const RemoteAsset &asset);

    // Convenience helpers
    QString makeUserAgent() const;

private:
    MainWindow *m_mainWindow = nullptr;
    QNetworkAccessManager *m_netMgr = nullptr;

    // For internal direct-download flow
    QNetworkReply *m_currentDownloadReply = nullptr;
    QFile *m_currentDownloadFile = nullptr;

    QString m_downloadDir;
    QList<RemoteRelease> m_releases;
};
