// mainwindow.cpp
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "version.h"
#include "updater.h"
#include "changeuser.h"

#include <QDebug>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QUrl>

QListWidget* MainWindow::remoteVersionsList() const { return ui->remoteVersionsList; }
QListWidget* MainWindow::filesToDownloadList() const { return ui->filesToDownloadList; }
QTextEdit*   MainWindow::releaseDescription() const { return ui->releaseDescription; }
QLabel*      MainWindow::currentBuildLabel() const { return ui->currentBuildLabel; }
QPushButton* MainWindow::downloadButton() const { return ui->downloadButton; }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_updater(nullptr)
    , m_netManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
    ui->setupUi(this);

    this->setWindowTitle("RJLauncher " + AppVersion::full());
    this->setFixedSize(800, 600);

    // Setup default directories inside application directory
    QString appDir = QCoreApplication::applicationDirPath();
    m_downloadedZipDir = QDir(appDir).filePath("DownloadedZip");
    m_sevenZipDir      = QDir(appDir).filePath("7zip");
    m_sevenZipExePath  = QDir(m_sevenZipDir).filePath("7za.exe"); // Windows default
#ifdef Q_OS_UNIX
    m_sevenZipExePath  = QDir(m_sevenZipDir).filePath("7za");     // Unix default
#endif

    // Ensure directories exist
    QDir().mkpath(m_downloadedZipDir);
    QDir().mkpath(m_sevenZipDir);

    // Create and initialize updater
    m_updater = new Updater(this);
    m_updater->initUpdater();
    m_updater->setupConnections();

    // Wire the Update-tab "Get Lib" button to call the internal direct 7zip downloader.
    if (ui->getLibButton) {
        connect(ui->getLibButton, &QPushButton::clicked, this, [this]() {
            ui->getLibButton->setEnabled(false);

            // Prefer Updater's download if available
            if (m_updater) {
                m_updater->downloadSevenZipDirectly();
            } else {
                qWarning() << "Updater instance missing; cannot download 7zip.";
                QMessageBox::warning(this, tr("Download"), tr("Updater not initialized; cannot download 7zip."));
            }

            // Safety fallback: re-enable after 15s if Updater didn't update UI
            QTimer::singleShot(15000, this, [this]() {
                if (ui->getLibButton) ui->getLibButton->setEnabled(true);
            });
        });
    } else {
        qWarning() << "MainWindow: getLibButton not found in UI. Ensure Update tab contains getLibButton.";
    }

    // Connect Download button to our handler
    if (ui->downloadButton) {
        connect(ui->downloadButton, &QPushButton::clicked, this, &MainWindow::on_downloadButton_clicked);
    } else {
        qWarning() << "MainWindow: downloadButton not found in UI.";
    }

    // Basic network error handling (additional per-reply connections are used when starting downloads)
    connect(m_netManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply){
        Q_UNUSED(reply);
    });
}

MainWindow::~MainWindow()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    delete m_updater;
    delete ui;
}

// Called when user clicks Download and Install Package
void MainWindow::on_downloadButton_clicked()
{
    // Determine selected version and download URL
    QListWidget *list = ui->remoteVersionsList;
    if (!list) {
        QMessageBox::warning(this, tr("Download"), tr("Remote versions list not available."));
        return;
    }

    QListWidgetItem *item = list->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Download"), tr("Please select a version to download."));
        return;
    }

    // Expect the download URL to be stored in item->data(Qt::UserRole)
    QString downloadUrl = item->data(Qt::UserRole).toString();
    if (downloadUrl.isEmpty()) {
        // Fallback: try the item text as URL or filename
        downloadUrl = item->text();
    }

    if (downloadUrl.isEmpty()) {
        QMessageBox::warning(this, tr("Download"), tr("No download URL found for the selected item."));
        return;
    }

    // Prepare destination path
    QString fileName = QFileInfo(QUrl(downloadUrl).path()).fileName();
    if (fileName.isEmpty()) fileName = "package.zip";
    QString destPath = QDir(m_downloadedZipDir).filePath(fileName);
    m_lastDownloadedZipPath = destPath;

    // Disable UI while downloading
    ui->downloadButton->setEnabled(false);

    // Start download
    QNetworkRequest request((QUrl(downloadUrl)));
    m_currentReply = m_netManager->get(request);

    // Stream to file
    QFile *outFile = new QFile(destPath);
    if (!outFile->open(QIODevice::WriteOnly)) {
        delete outFile;
        QMessageBox::critical(this, tr("Download"), tr("Failed to open file for writing: %1").arg(destPath));
        ui->downloadButton->setEnabled(true);
        return;
    }

    connect(m_currentReply, &QNetworkReply::readyRead, this, [this, outFile]() {
        if (m_currentReply)
            outFile->write(m_currentReply->readAll());
    });

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal){
        Q_UNUSED(bytesReceived);
        Q_UNUSED(bytesTotal);
        // Optionally update a progress bar here if you add one to the UI.
    });

    connect(m_currentReply, &QNetworkReply::finished, this, [this, outFile]() {
        outFile->close();
        outFile->deleteLater();

        if (!m_currentReply) {
            ui->downloadButton->setEnabled(true);
            return;
        }

        if (m_currentReply->error() != QNetworkReply::NoError) {
            QString err = m_currentReply->errorString();
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            QMessageBox::critical(this, tr("Download"), tr("Download failed: %1").arg(err));
            ui->downloadButton->setEnabled(true);
            return;
        }

        // Download succeeded
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        // Extract using 7za from m_sevenZipExePath
        QFileInfo sevenExe(m_sevenZipExePath);
        if (!sevenExe.exists()) {
            // 7za not found. Inform user and offer to use Updater to fetch it.
            QMessageBox::warning(this, tr("7zip missing"),
                                 tr("7za executable not found in %1.\nPlease click Get Lib to download 7zip, then try again.")
                                     .arg(m_sevenZipDir));
            ui->downloadButton->setEnabled(true);
            return;
        }

        // Create extraction target folder next to zip file (DownloadedZip/<zipname>_extracted)
        QString baseName = QFileInfo(m_lastDownloadedZipPath).baseName();
        QString extractTarget = QDir(m_downloadedZipDir).filePath(baseName + "_extracted");
        QDir().mkpath(extractTarget);

        // Build 7za command: x <zip> -o<extractTarget> -y
        QStringList args;
        args << "x" << m_lastDownloadedZipPath << "-o" + QDir::toNativeSeparators(extractTarget) << "-y";

        // Use QProcess to run 7za detached so extraction runs independently
        bool started = QProcess::startDetached(QDir::toNativeSeparators(m_sevenZipExePath), args, m_sevenZipDir);
        if (!started) {
            QMessageBox::critical(this, tr("Extraction"), tr("Failed to start 7za for extraction."));
            ui->downloadButton->setEnabled(true);
            return;
        }

        // Attempt to locate MadeChanges.exe inside the extracted folder.
        // Note: extraction is asynchronous; depending on package size you may want to wait or poll.
        QString madeChangesPath = QDir(extractTarget).filePath("MadeChanges.exe");
        QFileInfo madeInfo(madeChangesPath);

        // If not found immediately, try a simple search of top-level files (non-recursive)
        if (!madeInfo.exists()) {
            QDir d(extractTarget);
            QStringList found = d.entryList(QStringList() << "MadeChanges.exe", QDir::Files | QDir::NoSymLinks);
            if (!found.isEmpty()) {
                madeChangesPath = d.filePath(found.first());
            }
        }

        // Launch MadeChanges.exe detached if it exists
        if (QFileInfo::exists(madeChangesPath)) {
            bool launched = QProcess::startDetached(QDir::toNativeSeparators(madeChangesPath), QStringList(), extractTarget);
            if (!launched) {
                QMessageBox::warning(this, tr("Launch"), tr("Failed to launch MadeChanges.exe at %1").arg(madeChangesPath));
            } else {
                QMessageBox::information(this, tr("Install"), tr("Package installed and MadeChanges.exe launched."));
            }
        } else {
            QMessageBox::information(this, tr("Install"), tr("Package extracted to %1 but MadeChanges.exe was not found.").arg(extractTarget));
        }

        ui->downloadButton->setEnabled(true);
    });

    // Use the non-overloaded signal name available in Qt6
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &MainWindow::onNetworkError);
}

void MainWindow::onNetworkError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if (m_currentReply) {
        QString err = m_currentReply->errorString();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        QMessageBox::critical(this, tr("Network Error"), tr("Network error: %1").arg(err));
        ui->downloadButton->setEnabled(true);
    }
}

void MainWindow::on_ChangeUser_bta_clicked()
{
    ChangeUser *win2 = new ChangeUser(this);
    win2->setAttribute(Qt::WA_DeleteOnClose);
    win2->show();
}

void MainWindow::on_Setting_clicked()
{
    qDebug() << "Setting button clicked";
}
