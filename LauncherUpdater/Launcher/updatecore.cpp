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
#include <QLineEdit>
#include <QCheckBox>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <functional>
#include "Core.h"
#include "LauncherUpdater/LauncherDownload/downloadzip.h"

namespace FetchOS {
    QString getPlatformName();
}

QStringList DetectDownloadedPackages(); // Forward from zipdetector.cpp
QStringList DetectRollbackPackages();   // Forward from rollback.cpp
void TriggerRollback(const QString &oldFileName); // Forward from rollback.cpp
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
        
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir().mkpath(dataRoot + "Lib");
        QString destPath = dataRoot + "Lib/" + destName;

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
        
        // Center Top Search Box
        QLineEdit *searchPkg = new QLineEdit(this);
        searchPkg->setPlaceholderText("Search packages...");
        searchPkg->setFixedWidth(250);
        QHBoxLayout *searchPkgLayout = new QHBoxLayout();
        searchPkgLayout->addStretch();
        searchPkgLayout->addWidget(searchPkg);
        searchPkgLayout->addStretch();
        pkgLayout->addLayout(searchPkgLayout);

        QHBoxLayout *pkgHeader = new QHBoxLayout();
        pkgHeader->addWidget(new QLabel("Available Offline Packages", this));
        pkgHeader->addStretch();

        QListWidget *localZips = new QListWidget(this);
        pkgLayout->addWidget(localZips);

        // Filter logic for Tab 3
        auto applyPkgFilter = [localZips, searchPkg]() {
            QString text = searchPkg->text();
            for (int i = 0; i < localZips->count(); ++i) {
                QListWidgetItem *item = localZips->item(i);
                QWidget *w = localZips->itemWidget(item);
                if (w) {
                    QLabel *lbl = w->findChild<QLabel*>();
                    item->setHidden(!lbl->text().contains(text, Qt::CaseInsensitive));
                }
            }
        };
        connect(searchPkg, &QLineEdit::textChanged, applyPkgFilter);

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

        QPushButton *btnInstallLocal = new QPushButton("Apply Offline Update", this);
        btnInstallLocal->setEnabled(false);
        pkgLayout->addWidget(btnInstallLocal);

        // Custom setup logic for Tab 3 packages
        auto refreshTab3 = [localZips, btnInstallLocal, applyPkgFilter]() {
            localZips->clear();
            btnInstallLocal->setText("Apply Offline Update");
            btnInstallLocal->setStyleSheet("");
            btnInstallLocal->setEnabled(false);

            for (const QString &pkg : DetectDownloadedPackages()) {
                QListWidgetItem *item = new QListWidgetItem(localZips);
                item->setSizeHint(QSize(0, 38));

                QWidget *row = new QWidget();
                QHBoxLayout *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(10, 2, 10, 2);

                QLabel *nameLbl = new QLabel(pkg, row);
                rowLayout->addWidget(nameLbl, 1);

                QPushButton *insBtn = new QPushButton("Install", row);
                insBtn->setFixedWidth(65);
                insBtn->setVisible(false);
                rowLayout->addWidget(insBtn);

                QPushButton *delBtn = new QPushButton("Delete", row);
                delBtn->setFixedWidth(65);
                delBtn->setVisible(false); // Only visible when selected
                rowLayout->addWidget(delBtn);

                QFrame *sep = new QFrame(row);
                sep->setFrameShape(QFrame::VLine);
                sep->setFrameShadow(QFrame::Sunken);
                rowLayout->addWidget(sep);

                QCheckBox *chk = new QCheckBox(row);
                rowLayout->addWidget(chk);

                localZips->setItemWidget(item, row);

                // Action: Direct Install
                QObject::connect(insBtn, &QPushButton::clicked, [pkg]() {
                    TriggerLocalUpdate(pkg);
                });

                // Action: Direct delete on the line
                QObject::connect(delBtn, &QPushButton::clicked, [pkg, localZips, btnInstallLocal]() {
                    if (QMessageBox::question(nullptr, "Delete", "Remove " + pkg + "?") == QMessageBox::Yes) {
                        QString dataRoot = MinecraftLauncher::getRJLDataPath();
                        QFile::remove(dataRoot + "LauncherUpdater/LauncherSource/" + pkg);
                        for (int i = 0; i < localZips->count(); ++i) {
                            if (QWidget *w = localZips->itemWidget(localZips->item(i))) {
                                if (w->findChild<QLabel*>()->text() == pkg) {
                                    delete localZips->takeItem(i);
                                    break;
                                }
                            }
                        }
                    }
                });

                // Action: Monitor checkbox for bulk delete mode
                QObject::connect(chk, &QCheckBox::checkStateChanged, [localZips, btnInstallLocal](Qt::CheckState) {
                    int count = 0;
                    for (int i = 0; i < localZips->count(); ++i) {
                        QWidget *w = localZips->itemWidget(localZips->item(i));
                        if (w && w->findChild<QCheckBox*>()->isChecked()) count++;
                    }
                    if (count >= 2) {
                        btnInstallLocal->setText(QString("Delete Selected Packages (%1)").arg(count));
                        btnInstallLocal->setStyleSheet("color: white; background-color: #D32F2F; font-weight: bold;");
                        btnInstallLocal->setEnabled(true);
                    } else {
                        btnInstallLocal->setText("Apply Offline Update");
                        btnInstallLocal->setStyleSheet("");
                        btnInstallLocal->setEnabled(localZips->currentItem() != nullptr);
                    }
                });
            }
            applyPkgFilter();
        };

        // Handle individual row button visibility based on selection
        connect(localZips, &QListWidget::currentItemChanged, [localZips, btnInstallLocal](QListWidgetItem *curr, QListWidgetItem *prev) {
            if (prev) if (QWidget *w = localZips->itemWidget(prev)) {
                for (auto *btn : w->findChildren<QPushButton*>()) btn->hide();
            }
            if (curr) if (QWidget *w = localZips->itemWidget(curr)) {
                for (auto *btn : w->findChildren<QPushButton*>()) btn->show();
            }
            
            if (!btnInstallLocal->text().startsWith("Delete Selected")) {
                btnInstallLocal->setEnabled(curr != nullptr);
            }
        });

        // Handle main button logic (Apply vs Bulk Delete)
        connect(btnInstallLocal, &QPushButton::clicked, [localZips, refreshTab3, btnInstallLocal]() {
            if (btnInstallLocal->text().startsWith("Delete Selected")) {
                if (QMessageBox::warning(nullptr, "Confirm Bulk Delete", "Delete all selected ZIP files?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    QString dataRoot = MinecraftLauncher::getRJLDataPath();
                    for (int i = 0; i < localZips->count(); ++i) {
                        QWidget *w = localZips->itemWidget(localZips->item(i));
                        if (w && w->findChild<QCheckBox*>()->isChecked()) {
                            QFile::remove(dataRoot + "LauncherUpdater/LauncherSource/" + w->findChild<QLabel*>()->text());
                        }
                    }
                    refreshTab3();
                }
            } else {
                if (auto *item = localZips->currentItem()) {
                    if (QWidget *w = localZips->itemWidget(item))
                        TriggerLocalUpdate(w->findChild<QLabel*>()->text());
                }
            }
        });

        refreshTab3();
        updateTabs->addTab(pkgTab, "Downloaded Packages");

        // Tab 4: Rollback Update
        QWidget *rollbackTab = new QWidget();
        QVBoxLayout *rollLayout = new QVBoxLayout(rollbackTab);

        // Center Top Search Box
        QLineEdit *searchRoll = new QLineEdit(this);
        searchRoll->setPlaceholderText("Search backups...");
        searchRoll->setFixedWidth(250);
        QHBoxLayout *searchRollLayout = new QHBoxLayout();
        searchRollLayout->addStretch();
        searchRollLayout->addWidget(searchRoll);
        searchRollLayout->addStretch();
        rollLayout->addLayout(searchRollLayout);
        
        rollLayout->addWidget(new QLabel("Select a version to rollback to"));
        QListWidget *rollList = new QListWidget();
        rollLayout->addWidget(rollList);

        // Filter logic for Tab 4
        auto applyRollFilter = [rollList, searchRoll]() {
            QString text = searchRoll->text();
            for (int i = 0; i < rollList->count(); ++i) {
                QListWidgetItem *item = rollList->item(i);
                item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
            }
        };
        connect(searchRoll, &QLineEdit::textChanged, applyRollFilter);
        
        QPushButton *btnRollback = new QPushButton("Rollback to Selected Version");
        connect(btnRollback, &QPushButton::clicked, [rollList]() {
            if (auto item = rollList->currentItem()) {
                TriggerRollback(item->text());
            }
        });
        rollLayout->addWidget(btnRollback);
        updateTabs->addTab(rollbackTab, "Rollback Update");

        connect(updateTabs, &QTabWidget::currentChanged, [updateTabs, localZips, rollList, refreshTab3, applyRollFilter](int index) {
            if (updateTabs->tabText(index) == "Downloaded Packages") {
                refreshTab3();
            }
            else if (updateTabs->tabText(index) == "Rollback Update") {
                rollList->clear();
                rollList->addItems(DetectRollbackPackages());
                applyRollFilter();
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