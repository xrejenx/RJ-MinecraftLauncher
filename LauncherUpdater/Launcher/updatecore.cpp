#include <QWidget>
#include <QDialog>
#include <QTabWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QProgressBar>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <functional>
#include "LauncherUpdater/LauncherDownload/downloadzip.h"

namespace FetchOS {
    QString getPlatformName();
}

QStringList DetectDownloadedPackages(); // Forward from zipdetector.cpp
void TriggerLocalUpdate(const QString &localZipName); // From updater.cpp
void LogLauncherEvent(const QString &message); // From ConsoleOutput.cpp

/**
 * DownloadTask - Handles individual file downloads in a separate thread.
 */
class DownloadTask : public QObject {
    Q_OBJECT
public:
    DownloadTask(const QString &url, const QString &path, QObject *parent = nullptr)
        : QObject(parent), m_url(url), m_path(path), m_manager(nullptr), m_reply(nullptr) {}

    void startTask() {
        m_manager = new QNetworkAccessManager(this);
        QNetworkRequest request{QUrl(m_url)}; // Use uniform initialization to avoid "most vexing parse"
        m_reply = m_manager->get(request);

        connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadTask::onDownloadProgress);
        connect(m_reply, &QNetworkReply::finished, this, &DownloadTask::onDownloadFinished);
    }

signals:
    void taskProgress(qint64 bytesReceived, qint64 bytesTotal);
    void taskFinished(bool success, const QString &filePath);
    void taskError(const QString &errorString);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
        emit taskProgress(bytesReceived, bytesTotal);
    }

    void onDownloadFinished() {
        if (m_reply->error() == QNetworkReply::NoError) {
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(m_reply->readAll());
                file.close();
                emit taskFinished(true, m_path);
            } else {
                emit taskError("Failed to open file for writing: " + m_path);
                emit taskFinished(false, m_path);
            }
        } else {
            emit taskError("Download failed: " + m_reply->errorString());
            emit taskFinished(false, m_path);
        }
        m_reply->deleteLater();
        m_manager->deleteLater();
    }

private:
    QString m_url;
    QString m_path;
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply;
};

class SevenZipDownloaderDialog : public QDialog {
    Q_OBJECT
public:
    explicit SevenZipDownloaderDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("7zip Library Downloader");
        setFixedSize(400, 180);

        QVBoxLayout *layout = new QVBoxLayout(this);
        statusLabel = new QLabel("Status: Idle", this);
        layout->addWidget(statusLabel);

        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        layout->addWidget(progressBar);

        downloadBtn = new QPushButton("Download Library", this);
        layout->addWidget(downloadBtn);

        connect(downloadBtn, &QPushButton::clicked, this, &SevenZipDownloaderDialog::startDownload);
    }

private slots:
    void startDownload() {
        downloadBtn->setEnabled(false);
        statusLabel->setText("Status: Connecting...");

        QString os = FetchOS::getPlatformName();
        QString libUrl;
        QString destName;

        if (os == "windows") {
            libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Windows7zip/7za.exe";
            destName = "7za.exe";
        } else {
            libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Linux7zip/7za";
            destName = "7za";
        }

        QDir().mkpath("Lib");
        QString destPath = QDir::current().absoluteFilePath("Lib/" + destName);

        DownloadTask *task = new DownloadTask(libUrl, destPath);
        QThread *thread = new QThread(this);
        task->moveToThread(thread);

        connect(thread, &QThread::started, task, &DownloadTask::startTask);
        connect(task, &DownloadTask::taskProgress, this, [this](qint64 r, qint64 t) {
            if (t > 0) progressBar->setValue((r * 100) / t);
            statusLabel->setText(QString("Status: Downloading... (%1%)").arg(progressBar->value()));
        });

        connect(task, &DownloadTask::taskFinished, this, [this, task, thread, destPath](bool success, const QString &path) {
            if (success) {
                statusLabel->setText("Status: Success!");
                progressBar->setValue(100);
                QMessageBox::information(this, "7zip Library", "Library downloaded and installed successfully.");
#ifndef Q_OS_WIN
                QFile::setPermissions(destPath, QFile::permissions(destPath) | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
#endif
                accept();
            } else {
                statusLabel->setText("Status: Failed!");
                QMessageBox::critical(this, "Error", "Failed to download the 7zip library.");
                downloadBtn->setEnabled(true);
            }
            task->deleteLater();
            thread->quit();
            thread->wait();
            thread->deleteLater();
        });

        thread->start();
    }

private:
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QPushButton *downloadBtn;
};

class UpdateCoreUI : public QWidget {
public:
    UpdateCoreUI(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QTabWidget *updateTabs = new QTabWidget(this);

        // Tab 1: News (Fetch from GitHub Releases)
        QWidget *newsTab = new QWidget();
        QVBoxLayout *newsLayout = new QVBoxLayout(newsTab);
        QTextBrowser *newsBrowser = new QTextBrowser();
        newsBrowser->setObjectName("newsBrowser");
        newsBrowser->setPlaceholderText("Fetching latest news from GitHub...");
        newsLayout->addWidget(newsBrowser);
        updateTabs->addTab(newsTab, "News");

        // Tab 2: Update (Selectable Downloads)
        QWidget *updateTab = new QWidget();
        QVBoxLayout *upLayout = new QVBoxLayout(updateTab);
        QSplitter *upSplitter = new QSplitter(Qt::Horizontal);
        
        QListWidget *versionList = new QListWidget(upSplitter);
        versionList->setObjectName("updateList");
        
        QLabel *currentVerLabel = new QLabel(this);
        currentVerLabel->setObjectName("currentVerLabel");
        currentVerLabel->setStyleSheet("font-weight: bold; color: #555;");

        QWidget *rightPanel = new QWidget(upSplitter);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

        QTextBrowser *versionDetails = new QTextBrowser();
        versionDetails->setObjectName("updateDetails");
        versionDetails->setOpenExternalLinks(true);

        QListWidget *fileList = new QListWidget();
        fileList->setObjectName("fileList");

        rightLayout->addWidget(new QLabel("<b>Release Description:</b>"));
        rightLayout->addWidget(versionDetails, 1);
        rightLayout->addWidget(new QLabel("<b>Select Files to Download:</b>"));
        rightLayout->addWidget(fileList, 1);

        upLayout->addWidget(currentVerLabel);
        upLayout->addWidget(new QLabel("<b>Available Remote Versions:</b>"));
        upSplitter->addWidget(rightPanel);
        upLayout->addWidget(upSplitter);
        
        QPushButton *btnStartDownload = new QPushButton("Download and Install Package");
        btnStartDownload->setObjectName("installUpdateBtn");
        upLayout->addWidget(btnStartDownload);
        updateTabs->addTab(updateTab, "Update");

        // Tab 3: Downloaded Packages (Local ZIP detector)
        QWidget *pkgTab = new QWidget();
        QVBoxLayout *pkgLayout = new QVBoxLayout(pkgTab);

        QHBoxLayout *pkgHeader = new QHBoxLayout();
        pkgHeader->addWidget(new QLabel("Available Offline Packages", this));
        pkgHeader->addStretch();

        QLabel *libStatusLabel = new QLabel(this);
        libStatusLabel->setStyleSheet("font-weight: bold;");
        
        auto updateLibStatus = [libStatusLabel]() {
            bool installed = false;
#ifdef Q_OS_WIN
            installed = QFile::exists("Lib/7za.exe");
#else
            installed = QFile::exists("Lib/7za");
#endif
            libStatusLabel->setText(installed ? "7zipLib:Installed" : "7zipLib:Not installed");
            libStatusLabel->setStyleSheet(installed ? "color: green;" : "color: red;");
        };
        updateLibStatus();

        QPushButton *getLibBtn = new QPushButton("Get Lib", this);
        connect(getLibBtn, &QPushButton::clicked, this, [this, updateLibStatus]() {
            SevenZipDownloaderDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) updateLibStatus();
        });

        pkgHeader->addWidget(libStatusLabel);
        pkgHeader->addWidget(getLibBtn);
        pkgLayout->addLayout(pkgHeader);

        QListWidget *localZips = new QListWidget();
        localZips->addItems(DetectDownloadedPackages());
        pkgLayout->addWidget(localZips);
        
        QPushButton *btnInstallLocal = new QPushButton("Apply Offline Update");
        connect(btnInstallLocal, &QPushButton::clicked, [localZips]() {
            if (auto item = localZips->currentItem()) {
                TriggerLocalUpdate(item->text());
            }
        });

        pkgLayout->addWidget(btnInstallLocal);
        updateTabs->addTab(pkgTab, "Downloaded Packages");

        connect(updateTabs, &QTabWidget::currentChanged, [updateTabs, localZips](int index) {
            if (updateTabs->tabText(index) == "Downloaded Packages") {
                localZips->clear();
                localZips->addItems(DetectDownloadedPackages());
            }
        });

        layout->addWidget(updateTabs);
    }
};

class DownloadProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit DownloadProgressDialog(const QList<QPair<QString, QString>>& filesToDownload, QWidget *parent = nullptr)
        : QDialog(parent), m_filesToDownload(filesToDownload), m_completedDownloads(0) {
        setWindowTitle("Downloading Updates");
        setFixedSize(500, 400);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        m_overallProgressBar = new QProgressBar(this);
        m_overallProgressBar->setRange(0, 100);
        m_overallProgressBar->setValue(0);
        mainLayout->addWidget(new QLabel("Overall Progress:"));
        mainLayout->addWidget(m_overallProgressBar);

        m_fileProgressList = new QListWidget(this);
        mainLayout->addWidget(new QLabel("Individual File Progress:"));
        mainLayout->addWidget(m_fileProgressList);

        m_cancelButton = new QPushButton("Cancel", this);
        mainLayout->addWidget(m_cancelButton);

        connect(m_cancelButton, &QPushButton::clicked, this, &DownloadProgressDialog::onCancelClicked);

        startDownloads();
    }

signals:
    void allDownloadsFinished(bool success, const QList<QString>& downloadedFiles);
    void downloadCancelled(const QList<QString>& partiallyDownloadedFiles);

private slots:
    void startDownloads() {
        m_overallProgressBar->setRange(0, m_filesToDownload.size());
        m_overallProgressBar->setValue(0);

        for (const auto& filePair : m_filesToDownload) {
            QString url = filePair.first;
            QString destination = filePair.second;
            QString fileName = QFileInfo(destination).fileName();

            QListWidgetItem *item = new QListWidgetItem(fileName + ": Initializing...", m_fileProgressList);
            QProgressBar *progressBar = new QProgressBar(m_fileProgressList);
            progressBar->setRange(0, 100);
            progressBar->setValue(0);
            m_fileProgressList->setItemWidget(item, progressBar);

            DownloadTask *task = new DownloadTask(url, destination);
            QThread *thread = new QThread(this); // Create a new thread for each download
            task->moveToThread(thread);

            connect(thread, &QThread::started, task, &DownloadTask::startTask);
            connect(task, &DownloadTask::taskProgress, this, [progressBar](qint64 received, qint64 total) {
                if (total > 0) progressBar->setValue((received * 100) / total);
            });
            connect(task, &DownloadTask::taskFinished, this, [this, item, fileName, task, thread, destination](bool success, const QString& filePath) {
                m_completedDownloads++;
                m_overallProgressBar->setValue(m_completedDownloads);
                if (success) {
                    item->setText(fileName + ": Downloaded!");
                    m_downloadedFiles.append(filePath);
                } else {
                    item->setText(fileName + ": Failed!");
                    m_failedDownloads.append(filePath);
                }
                task->deleteLater();
                thread->quit();
                thread->wait();
                thread->deleteLater();

                if (m_completedDownloads == m_filesToDownload.size()) {
                    emit allDownloadsFinished(m_failedDownloads.isEmpty(), m_downloadedFiles);
                    accept(); // Close dialog
                }
            });
            connect(task, &DownloadTask::taskError, this, [item, fileName](const QString& error) {
                item->setText(fileName + ": Error - " + error);
                LogLauncherEvent("Download error for " + fileName + ": " + error);
            });

            thread->start();
        }
    }

    void onCancelClicked() {
        // Stopping active downloads would require more complex management of the QNetworkReply objects
        // For now, we just emit the cancelled signal and close the dialog.
        emit downloadCancelled(m_downloadedFiles + m_failedDownloads); // Include partially downloaded
        reject();
    }

private:
    QList<QPair<QString, QString>> m_filesToDownload; // url, destinationPath
    QProgressBar *m_overallProgressBar;
    QListWidget *m_fileProgressList;
    QPushButton *m_cancelButton;
    int m_completedDownloads;
    QList<QString> m_downloadedFiles;
    QList<QString> m_failedDownloads;
};

#include "updatecore.moc"
QWidget* CreateModernUpdateTab(QWidget *parent) {
    return new UpdateCoreUI(parent);
}

QDialog* CreateDownloadProgressDialog(const QList<QPair<QString, QString>>& filesToDownload, QWidget *parent) {
    return new DownloadProgressDialog(filesToDownload, parent);
}

void ConnectDownloadDialogSignals(QDialog *dialog, QObject *receiver, 
                                  std::function<void(bool, const QList<QString>&)> finishedCb, 
                                  std::function<void(const QList<QString>&)> cancelledCb) {
    auto *pDialog = dynamic_cast<DownloadProgressDialog*>(dialog);
    if (pDialog) {
        QObject::connect(pDialog, &DownloadProgressDialog::allDownloadsFinished, receiver, 
            [finishedCb](bool success, const QList<QString>& files) { finishedCb(success, files); });
        QObject::connect(pDialog, &DownloadProgressDialog::downloadCancelled, receiver, 
            [cancelledCb](const QList<QString>& files) { cancelledCb(files); });
    }
}