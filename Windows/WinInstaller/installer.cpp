#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QCheckBox>
#include <QProgressDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDirIterator>
#include "LauncherUpdater/LauncherDownload/downloadzip.h" // For ExtractZipFile

// Forward declarations from version.cpp
QString GetLauncherTitle();
QString GetBuildTypePrefix();
int GetBuildNumber();

struct ReleaseAsset {
    QString name;
    QString url;
    qint64 size;
};
struct ReleaseInfo {
    int buildNumber;
    QString tagName;
    QList<ReleaseAsset> assets;
};
struct UpdateInfo { QList<ReleaseInfo> allReleases; };
UpdateInfo CheckForInstallerUpdates();

class DownloadTask : public QObject {
    Q_OBJECT
public:
    DownloadTask(const QString &url, const QString &path, QObject *parent = nullptr)
        : QObject(parent), m_url(url), m_path(path), m_manager(nullptr), m_reply(nullptr) {}

    void startTask() {
        m_manager = new QNetworkAccessManager(this);
        QNetworkRequest request{QUrl(m_url)};
        m_reply = m_manager->get(request);

        connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadTask::taskProgress);
        connect(m_reply, &QNetworkReply::finished, this, &DownloadTask::onDownloadFinished);
    }

signals:
    void taskProgress(qint64 bytesReceived, qint64 bytesTotal);
    void taskFinished(bool success, const QString &filePath);

private slots:
    void onDownloadFinished() {
        if (m_reply->error() == QNetworkReply::NoError) {
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(m_reply->readAll());
                file.close();
                emit taskFinished(true, m_path);
            } else {
                emit taskFinished(false, m_path);
            }
        } else {
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

class RJLInstaller : public QDialog {
    Q_OBJECT
public:
    RJLInstaller(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(GetLauncherTitle() + " - Setup");
        setFixedSize(650, 420);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Sidebar
        QWidget *sidebar = new QWidget(this);
        sidebar->setFixedWidth(220);
        sidebar->setStyleSheet("background-color: #000000; border-right: 1px solid #111;");
        QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);

        QLabel *iconLabel = new QLabel(this);
        iconLabel->setObjectName("sideLogo");
        QPixmap pix;
        if (QFile::exists("Windows/RJINSTICON.ico")) {
            pix = QIcon("Windows/RJINSTICON.ico").pixmap(128, 128);
        } else {
            pix.load(":/CmakeLauncherIcon/release/icon.png");
        }
        iconLabel->setPixmap(pix.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setAlignment(Qt::AlignCenter);

        sidebarLayout->addSpacing(40);
        sidebarLayout->addWidget(iconLabel);

        QLabel *sideTitle = new QLabel("RJ Launcher\nInstallation", this);
        sideTitle->setStyleSheet("color: white; font-size: 20px; font-weight: bold;");
        sideTitle->setAlignment(Qt::AlignCenter);

        sidebarLayout->addWidget(sideTitle);
        sidebarLayout->addSpacing(40);

        // Content Area
        QWidget *contentArea = new QWidget(this);
        QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
        contentLayout->setContentsMargins(25, 25, 25, 15);

        pages = new QStackedWidget(this);

        // Page 1
        QWidget *page1 = new QWidget();
        QVBoxLayout *p1Layout = new QVBoxLayout(page1);
        p1Layout->addWidget(new QLabel("<h2 style='color:#004400;'>Welcome to RJ Launcher</h2>", this));
        p1Layout->addWidget(new QLabel("Before we begin, would you like the installer to check for a newer version of itself?", this));
        rbUpdateYes = new QRadioButton("Check for installer updates (Recommended)", this);
        rbUpdateNo = new QRadioButton("Skip and continue with current installer", this);
        rbUninstall = new QRadioButton("Uninstall RJ Launcher from this system", this);
        rbUpdateYes->setChecked(true);
        p1Layout->addWidget(rbUpdateYes);
        p1Layout->addWidget(rbUpdateNo);
        p1Layout->addWidget(rbUninstall);
        p1Layout->addStretch();
        pages->addWidget(page1);

        // Page 2
        QWidget *page2 = new QWidget();
        QVBoxLayout *p2Layout = new QVBoxLayout(page2);
        p2Layout->addWidget(new QLabel("<h2 style='color:#004400;'>Installation Folder</h2>", this));
        p2Layout->addWidget(new QLabel("Setup will install RJ Launcher in the following folder.\nTo install in a different folder, click Browse.", this));
        QHBoxLayout *pathLayout = new QHBoxLayout();
        pathEdit = new QLineEdit("C:/Program Files/RJLauncher", this);
        QPushButton *browseBtn = new QPushButton("Browse...", this);
        pathLayout->addWidget(pathEdit);
        pathLayout->addWidget(browseBtn);
        p2Layout->addLayout(pathLayout);
        p2Layout->addStretch();
        pages->addWidget(page2);

        connect(browseBtn, &QPushButton::clicked, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Installation Directory", pathEdit->text());
            if (!dir.isEmpty()) pathEdit->setText(QDir::toNativeSeparators(dir));
        });

        // Page 3
        QWidget *page3 = new QWidget();
        QVBoxLayout *p3Layout = new QVBoxLayout(page3);
        p3Layout->addWidget(new QLabel("<h2 style='color:#004400;'>Final Steps</h2>", this));
        cbShortcut = new QCheckBox("Create Desktop Shortcut", this);
        cbLaunch = new QCheckBox("Launch RJ Launcher immediately after install", this);
        cbShortcut->setChecked(true);
        cbLaunch->setChecked(true);
        p3Layout->addWidget(cbShortcut);
        p3Layout->addWidget(cbLaunch);
        p3Layout->addStretch();
        pages->addWidget(page3);

        contentLayout->addWidget(pages);

        // Navigation
        QHBoxLayout *navLayout = new QHBoxLayout();
        btnBack = new QPushButton("< Back", this);
        btnNext = new QPushButton("Next >", this);
        QPushButton *btnCancel = new QPushButton("Cancel", this);
        navLayout->addStretch();
        navLayout->addWidget(btnBack);
        navLayout->addWidget(btnNext);
        navLayout->addSpacing(10);
        navLayout->addWidget(btnCancel);
        contentLayout->addLayout(navLayout);

        btnBack->setEnabled(false);

        mainLayout->addWidget(sidebar);
        mainLayout->addWidget(contentArea);

        connect(btnNext, &QPushButton::clicked, this, &RJLInstaller::handleNext);
        connect(btnBack, &QPushButton::clicked, this, &RJLInstaller::handleBack);
        connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);

        checkExistingInstall();
    }

private slots:
    void handleNext() {
        int idx = pages->currentIndex();
        
        if (idx == 0) {
            if (rbUpdateYes->isChecked()) {
                QProgressDialog *prog = new QProgressDialog("Checking for updates...", "Cancel", 0, 100, this);
                prog->setWindowModality(Qt::WindowModal);
                prog->setMinimumDuration(0);
                prog->show();
                QApplication::processEvents();

                UpdateInfo ui = CheckForInstallerUpdates();
                if (!ui.allReleases.isEmpty()) {
                    int latest = ui.allReleases.first().buildNumber;
                    if (latest > GetBuildNumber()) {
                        prog->setLabelText("Downloading updated installer...");
                        QString updateUrl = ui.allReleases.first().assets.first().url;
                        QString dest = QDir::tempPath() + "/" + ui.allReleases.first().assets.first().name;
                        
                        DownloadTask *task = new DownloadTask(updateUrl, dest, this);
                        connect(task, &DownloadTask::taskProgress, prog, [prog](qint64 r, qint64 t){
                            if (t > 0) prog->setValue((r * 100) / t);
                        });
                        
                        QEventLoop downloadLoop;
                        connect(task, &DownloadTask::taskFinished, &downloadLoop, [&downloadLoop](bool, const QString&){ downloadLoop.quit(); });
                        connect(prog, &QProgressDialog::canceled, &downloadLoop, &QEventLoop::quit);
                        task->startTask();
                        downloadLoop.exec();
                        
                        if (!prog->wasCanceled()) {
                            QProcess::startDetached(dest);
                            qApp->quit();
                            return;
                        }
                    } else {
                        QMessageBox::information(this, "Up to Date", "Your installer is already the latest version.");
                    }
                }
                prog->deleteLater();
            }
            if (rbUninstall->isChecked()) {
                performUninstall();
                return;
            }
        }

        if (idx < 2) {
            pages->setCurrentIndex(idx + 1);
            btnBack->setEnabled(true);
            if (idx + 1 == 2) btnNext->setText("Install");
        } else {
            performInstall();
        }
    }

    void handleBack() {
        int idx = pages->currentIndex();
        if (idx > 0) {
            pages->setCurrentIndex(idx - 1);
            btnNext->setText("Next >");
            if (idx - 1 == 0) btnBack->setEnabled(false);
        }
    }

private:
    void checkExistingInstall() {
        if (QDir(pathEdit->text()).exists()) {
            rbUninstall->setEnabled(true);
        } else {
            rbUninstall->setEnabled(false);
            if (rbUninstall->isChecked()) rbUpdateYes->setChecked(true);
        }
    }

    void performInstall() {
        QString instPath = pathEdit->text();
        QDir().mkpath(instPath);

        auto moveAndFlatten = [](const QString &srcPath, const QString &dstPath) {
            QDir srcDir(srcPath);
            QStringList entries = srcDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
            QString finalSrc = srcPath;
            if (entries.size() == 1 && QFileInfo(srcDir.absoluteFilePath(entries[0])).isDir()) {
                finalSrc = srcDir.absoluteFilePath(entries[0]);
            }
            QDir source(finalSrc);
            for (const QString &f : source.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden)) {
                QString dest = QDir(dstPath).absoluteFilePath(f);
                if (QFile::exists(dest)) { if (QFileInfo(dest).isDir()) QDir(dest).removeRecursively(); else QFile::remove(dest); }
                QFile::rename(source.absoluteFilePath(f), dest);
            }
        };

        // External tools cannot read from Qt Resources (":/"), so we must copy it to a temp file first
        QString zipResourcePath = ":/Windows/WinInstaller/Payload/app_data.zip";
        QString tempZipPath = QDir::tempPath() + "/RJML_install_payload.zip";

        if (QFile::exists(zipResourcePath)) {
            if (QFile::exists(tempZipPath)) QFile::remove(tempZipPath); // Clean old attempts

            QString tempExtract = QDir::tempPath() + "/RJML_install_extract";
            QDir(tempExtract).removeRecursively();
            QDir().mkpath(tempExtract);

            if (QFile::copy(zipResourcePath, tempZipPath)) {
                if (ExtractZipFile(tempZipPath, tempExtract)) {
                    moveAndFlatten(tempExtract, instPath);
                    QMessageBox::information(this, "Success", "Installation completed successfully!");
                    if (cbLaunch->isChecked()) {
                        QProcess::startDetached(instPath + "/RJML.exe");
                    }
                    QFile::remove(tempZipPath); // Cleanup temp file
                    QDir(tempExtract).removeRecursively();
                    accept();
                    return;
                }
            }
        } else {
            QMessageBox::critical(this, "Error", "Installation payload not found inside the installer.");
            return;
        }
        QMessageBox::critical(this, "Error", "Failed to extract application data. Please check permissions.");
    }

    void performUninstall() {
        // Logic to remove files from pathEdit->text()
        QMessageBox::information(this, "Uninstall", "Uninstall feature logic would execute here.");
    }

    QStackedWidget *pages;
    QRadioButton *rbUpdateYes, *rbUpdateNo, *rbUninstall;
    QLineEdit *pathEdit;
    QCheckBox *cbShortcut, *cbLaunch;
    QPushButton *btnBack, *btnNext;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    RJLInstaller w;
    w.show();
    return a.exec();
}

#include "installer.moc"
