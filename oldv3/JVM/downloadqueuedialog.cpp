#include "downloadqueuedialog.h"
#include "jvmdownloadernew.h" // For JavaVersionItemWidget
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, LogLauncherEvent

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSysInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QStyleFactory>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QApplication> // For QApplication::processEvents()

// External logger interface (defined in Core.h, implemented elsewhere)
extern void LogLauncherEvent(const QString &message);

DownloadQueueDialog::DownloadQueueDialog(const QList<QListWidgetItem*>& itemsToDownload, QWidget *parent)
    : QDialog(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_completedCount(0),
      m_failedCount(0),
      m_maxConcurrentDownloads(5) // Increased concurrent downloads
{
    setWindowTitle("Download Queue");
    setFixedSize(700, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Overall Progress:"));
    m_overallProgressBar = new QProgressBar(this);
    m_overallProgressBar->setRange(0, itemsToDownload.size());
    m_overallProgressBar->setValue(0);
    m_overallProgressBar->setTextVisible(true);
    m_overallProgressBar->setFormat("%v/%m downloads completed");
    mainLayout->addWidget(m_overallProgressBar);

    mainLayout->addWidget(new QLabel("Download Queue:"));
    m_logList = new QListWidget(this);
    m_logList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // No scrollbar for fixed 5-item look
    mainLayout->addWidget(m_logList);

    m_cancelAllButton = new QPushButton("Cancel All", this);
    mainLayout->addWidget(m_cancelAllButton);
    connect(m_cancelAllButton, &QPushButton::clicked, this, &DownloadQueueDialog::onCancelAllClicked);

    // Populate the download queue
    // We no longer create widgets here to keep them "hidden" until they start
    for (QListWidgetItem* item : itemsToDownload) {
        DownloadInfo* info = new DownloadInfo();
        info->originalItem = item;
        info->downloadUrl = item->data(Qt::UserRole).toString();

        QVariant secondData = item->data(Qt::UserRole + 1);
        if (secondData.canConvert<QJsonObject>()) {
            // Java Download Setup
            QJsonObject pkg = secondData.value<QJsonObject>();
            QString javaVer = pkg["java_version"].toArray().first().toVariant().toString();
            QString javaName = "zulu-jdk-" + (javaVer.isEmpty() ? "unknown" : javaVer);
            QString ext = (QSysInfo::kernelType() == "winnt") ? ".zip" : ".tar.gz";
            info->destPath = MinecraftLauncher::getRJLDataPath() + "javas/" + javaName + ext;
            info->isJava = true;
        } else {
            // Generic Component (Minecraft JAR, Asset, Library)
            info->destPath = secondData.toString();
            info->isJava = false;
        }

        info->reply = nullptr; // Will be set when download starts
        m_allDownloads.append(info);
        m_downloadQueue.enqueue(info);
    }

    // Start downloads after UI is set up
    QTimer::singleShot(0, this, &DownloadQueueDialog::startDownloads);
}

DownloadQueueDialog::~DownloadQueueDialog() {
    m_isClosing = true;
    // Abort any active downloads
    for (auto it = m_activeDownloads.begin(); it != m_activeDownloads.end(); ++it) {
        if (it.key()->isRunning()) {
            it.key()->abort();
        }
        it.key()->deleteLater();
    }
    // Clean up our heap-allocated info objects
    qDeleteAll(m_allDownloads);
}

void DownloadQueueDialog::startDownloads() {
    processNextDownload(); // Start the first batch
}

void DownloadQueueDialog::processNextDownload() {
    if (m_isClosing) return;
    while (!m_downloadQueue.isEmpty() && m_activeDownloads.size() < m_maxConcurrentDownloads) {
        DownloadInfo* info = m_downloadQueue.dequeue();
        if (!info) continue;

        // Create UI Item only when starting (Slide-In / Fade-In)
        QListWidgetItem* queueItem = new QListWidgetItem(m_logList);
        queueItem->setSizeHint(QSize(m_logList->width(), 40));
        info->queueItem = queueItem;

        QWidget *itemRowWidget = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(itemRowWidget);
        rowLayout->setContentsMargins(10, 0, 10, 0);

        QString displayName = info->originalItem ? info->originalItem->text() : "Task";
        QLabel *nameLabel = new QLabel(displayName, itemRowWidget);
        info->progressBar = new QProgressBar(itemRowWidget);
#ifdef Q_OS_WIN
        info->progressBar->setFixedHeight(24);
#endif
        info->progressBar->setRange(0, 100);
        info->progressBar->setMinimumWidth(180); 
        info->progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        info->progressBar->setAlignment(Qt::AlignCenter);
        info->progressBar->setTextVisible(true);

        rowLayout->addWidget(nameLabel, 1);
        rowLayout->addWidget(info->progressBar);

        // Apply Opacity Effect for Fade-In
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(itemRowWidget);
        eff->setOpacity(0.0);
        itemRowWidget->setGraphicsEffect(eff);
        m_logList->setItemWidget(queueItem, itemRowWidget);

        // Enforce 10-item limit by hiding old finished items
        int visibleCount = 0;
        // Iterate backwards to keep the most recent items visible
        for (int i = m_logList->count() - 1; i >= 0; --i) {
            QListWidgetItem* it = m_logList->item(i);
            if (!it->isHidden()) {
                visibleCount++;
                if (visibleCount > 10 && it->data(Qt::UserRole + 10).toBool()) it->setHidden(true);
            }
        }

        // Animation: Fade in
        QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        if (info->progressBar) info->progressBar->setFormat("Connecting...");
        LogLauncherEvent("Starting download from " + info->downloadUrl + " to " + info->destPath);

        QNetworkRequest request(QUrl(info->downloadUrl));
        info->reply = m_networkManager->get(request);
        m_activeDownloads[info->reply] = info;

        connect(info->reply, &QNetworkReply::downloadProgress, this, &DownloadQueueDialog::onDownloadProgress);
        connect(info->reply, &QNetworkReply::finished, this, &DownloadQueueDialog::onDownloadFinished);
    }
}

void DownloadQueueDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || m_isClosing) return;

    DownloadInfo* info = m_activeDownloads.value(reply);
    if (!info || !info->progressBar) return;

    if (bytesTotal > 0) {
        if (info->progressBar->maximum() == 0) info->progressBar->setRange(0, 100);
        int progress = (bytesReceived * 100) / bytesTotal;
        info->progressBar->setValue(progress);
        info->progressBar->setFormat(QString("Downloading: %1%").arg(progress));
    } else {
        // Handle unknown file size: Indeterminate progress
        info->progressBar->setRange(0, 0); 
        info->progressBar->setFormat(QString("%1 KB").arg(bytesReceived / 1024));
    }
}

void DownloadQueueDialog::onDownloadFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || m_isClosing) return;

    DownloadInfo* info = m_activeDownloads.take(reply);
    if (!info) return;

    if (reply->error() != QNetworkReply::NoError && info->retryCount < 5) {
        info->retryCount++;
        LogLauncherEvent(QString("Download failed: %1. Retry %2/5 in 5s...").arg(reply->errorString()).arg(info->retryCount));
        if (info->progressBar) info->progressBar->setFormat(QString("Retrying in 5s (%1/5)...").arg(info->retryCount));
        
        reply->deleteLater();
        QTimer::singleShot(5000, this, [this, info]() {
            m_downloadQueue.enqueue(info);
            processNextDownload();
        });
        return;
    }

    bool success = false;
    QString statusMessage;

    if (reply->error() == QNetworkReply::NoError) {
        QString filePath = info->destPath;
        QDir().mkpath(QFileInfo(filePath).absolutePath());

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            QApplication::processEvents(); // Keep UI alive during mass file writing (assets)
            LogLauncherEvent("Downloaded to: " + filePath);

            // Handle Extraction: Only for Java or Natives
            bool needsExtraction = info->isJava || filePath.contains("/natives/");
            if (needsExtraction) {
                info->progressBar->setRange(0, 0);
                info->progressBar->setFormat("Extracting...");

                QString extractDir = QFileInfo(filePath).absolutePath();
                if (info->isJava) extractDir = MinecraftLauncher::getRJLDataPath() + "javas";

                QProcess *process = new QProcess(this);
                QString sevenZipPath = MinecraftLauncher::getRJLDataPath() + "Lib/" + 
#ifdef Q_OS_WIN
                    "7za.exe";
#else
                    "7za";
#endif

                if (QFile::exists(sevenZipPath)) {
                    process->start(sevenZipPath, {"x", filePath, "-o" + extractDir, "-y"});
                } else {
#ifdef Q_OS_WIN
                    process->start("tar", {"-xf", QDir::toNativeSeparators(filePath), "-C", QDir::toNativeSeparators(extractDir)});
#else
                    process->start("tar", {"-xzf", filePath, "-C", extractDir});
#endif
                }

                // Optimized: Use local event loop to wait without blocking the GUI
                QEventLoop extractionLoop;
                connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &extractionLoop, &QEventLoop::quit);
                connect(process, &QProcess::errorOccurred, &extractionLoop, &QEventLoop::quit);
                
                // 10-minute safety timeout for very slow HDDs
                QTimer::singleShot(600000, &extractionLoop, &QEventLoop::quit);

                extractionLoop.exec();

                if (process->exitCode() == 0 && process->exitStatus() == QProcess::NormalExit) {
                    QFile::remove(filePath); // Cleanup
                    success = true;
                    statusMessage = "Done";
                } else {
                    LogLauncherEvent("Extraction failed: " + process->readAllStandardError());
                    statusMessage = "Ext. Failed";
                    QFile::remove(filePath); // Clean up downloaded file
                }
                process->deleteLater();
            } else {
                success = true;
                statusMessage = "Done";
            }
            QApplication::processEvents();
        } else {
            LogLauncherEvent("Failed to open file for writing: " + filePath);
            statusMessage = "File Write Failed";
            QFile::remove(filePath); // Clean up partially written file
        }
    } else {
        LogLauncherEvent("Download failed for " + info->downloadUrl + ": " + reply->errorString());
        statusMessage = "Download Failed: " + reply->errorString();
        QFile::remove(info->destPath); // Clean up partially downloaded file
    }

    if (success) {
        m_completedCount++;
        if (info->progressBar) {
            info->progressBar->setFormat("Done!");
            info->progressBar->setValue(100);
        }
        if (info->queueItem) info->queueItem->setData(Qt::UserRole + 10, true); // Mark as finished

        // Slide-Out Animation
        if (!m_isClosing && info->queueItem && m_logList->itemWidget(info->queueItem)) {
            QWidget* widget = m_logList->itemWidget(info->queueItem);
            QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(widget);
            widget->setGraphicsEffect(eff);
            
            QPropertyAnimation* anim = new QPropertyAnimation(eff, "opacity");
            anim->setDuration(400);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            connect(anim, &QPropertyAnimation::finished, [item = info->queueItem](){
                if (item) item->setHidden(true);
            });
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }

        // Update original item in JVMDownloaderNew
        // This part is specific to JVMDownloaderNew's list items.
        // For generic downloads (like MC assets), originalItem->listWidget() will be null.
        // The Inst classes are responsible for updating their own UI elements.
    } else {
        m_failedCount++;
        info->progressBar->setFormat(statusMessage);
        info->progressBar->setStyleSheet("QProgressBar::chunk { background-color: red; }");
    }
    updateOverallProgress();

    reply->deleteLater();

    // Process next download in queue
    processNextDownload();

    if (m_downloadQueue.isEmpty() && m_activeDownloads.isEmpty()) {
        accept(); // Silent close meow!
    }
}

void DownloadQueueDialog::onCancelAllClicked() {
    m_isClosing = true; // Block further UI updates
    m_downloadQueue.clear(); // Clear pending downloads

    // Wipe the instance folder ONCE if needed, instead of inside the loop
    bool instanceWiped = false;
    for (DownloadInfo* info : m_allDownloads) {
        if (!instanceWiped && !info->isJava && info->destPath.contains("/Instances/")) {
            QString root = MinecraftLauncher::getRJLDataPath() + "Instances/";
            int idx = info->destPath.indexOf("/Instances/") + 11;
            QString name = info->destPath.mid(idx).split('/').first();
            QDir(root + name).removeRecursively();
            instanceWiped = true;
        }

        // Abort the network request first
        if (info->reply) {
            disconnect(info->reply, nullptr, this, nullptr);
            if (info->reply->isRunning()) info->reply->abort();
        }
        // Delete the partial file if it exists
        if (QFile::exists(info->destPath)) {
            QFile::remove(info->destPath);
        }
    }
    QList<QNetworkReply*> activeReplies = m_activeDownloads.keys();
    for (QNetworkReply* reply : activeReplies) {
        reply->disconnect(); // Stop it from calling onDownloadFinished
        if (reply->isRunning()) {
            reply->abort();
        }
        reply->deleteLater();
    }
    m_activeDownloads.clear();
    reject(); // Close the dialog
}

void DownloadQueueDialog::updateOverallProgress() {
    m_overallProgressBar->setValue(m_completedCount + m_failedCount);
    m_overallProgressBar->setFormat(QString("%1/%2 downloads completed (%3 failed)").arg(m_completedCount).arg(m_allDownloads.size()).arg(m_failedCount));
}
