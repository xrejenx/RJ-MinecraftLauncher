#ifndef DOWNLOADQUEUEDIALOG_H
#define DOWNLOADQUEUEDIALOG_H

#include <QDialog>
#include <QList>
#include <QQueue>
#include <QMap>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QListWidget>
#include <QPushButton>

struct DownloadInfo {
    QListWidgetItem* originalItem = nullptr;
    QListWidgetItem* queueItem = nullptr;
    QString downloadUrl;
    QString destPath;
    bool isJava = false;
    QNetworkReply* reply = nullptr;
    QProgressBar* progressBar = nullptr;
    int retryCount = 0;
};

class DownloadQueueDialog : public QDialog {
    Q_OBJECT
public:
    explicit DownloadQueueDialog(const QList<QListWidgetItem*>& itemsToDownload, QWidget *parent = nullptr);
    ~DownloadQueueDialog();

signals:
    void allDownloadsFinished(bool success, const QList<QString>& downloadedFiles);
    void downloadCancelled(const QList<QString>& partiallyDownloadedFiles);

private slots:
    void startDownloads();
    void processNextDownload();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onCancelAllClicked();

private:
    void updateOverallProgress();

    QNetworkAccessManager *m_networkManager;
    QQueue<DownloadInfo*> m_downloadQueue;
    QList<DownloadInfo*> m_allDownloads;
    QMap<QNetworkReply*, DownloadInfo*> m_activeDownloads;
    
    QProgressBar *m_overallProgressBar;
    QListWidget *m_logList;
    QPushButton *m_cancelButton;
    QPushButton *m_cancelAllButton;
    int m_completedCount;
    int m_failedCount;
    int m_maxConcurrentDownloads;
    bool m_isClosing = false;
};

#endif // DOWNLOADQUEUEDIALOG_H