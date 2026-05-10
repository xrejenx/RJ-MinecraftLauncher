#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDirIterator>
#include <QProgressBar>
#include <QTimer>

#include "Core.h"
#include "LauncherUpdater/LauncherDownload/downloadzip.h" // For ExtractZipFile

// External logger (from ConsoleOutput.cpp)
void LogLauncherEvent(const QString &message);
int GetBuildNumber(); // From version.cpp

// Fallback if the build system doesn't provide the build type macro
#ifndef LAUNCHER_BUILD_TYPE_SHORT
#define LAUNCHER_BUILD_TYPE_SHORT "r"
#endif

class MadeChangesDialog : public QDialog {
    Q_OBJECT
public:
    explicit MadeChangesDialog(const QString &backupZipToRestore = "", QWidget *parent = nullptr)
        : QDialog(parent), m_backupZipToRestore(backupZipToRestore) {
        setWindowTitle("RJ Launcher Rollback / Maintenance");
        setFixedSize(500, 300);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        m_statusLabel = new QLabel("<h2>RJ Launcher Maintenance</h2>", this);
        mainLayout->addWidget(m_statusLabel);
        
        m_infoLabel = new QLabel("Select a backup to restore or perform maintenance:", this);
        mainLayout->addWidget(m_infoLabel);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 0); // Indeterminate by default
        m_progressBar->setVisible(false);
        mainLayout->addWidget(m_progressBar);

        m_backupList = new QListWidget(this);
        m_backupList->setVisible(true);
        mainLayout->addWidget(m_backupList);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        m_rollbackButton = new QPushButton("Restore Selected Backup", this);
        m_cancelButton = new QPushButton("Cancel", this);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_rollbackButton);
        buttonLayout->addWidget(m_cancelButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_rollbackButton, &QPushButton::clicked, this, &MadeChangesDialog::performRollback);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        // Parse command line for automatic modes
        QStringList args = QCoreApplication::arguments();
        if (args.contains("--update") && args.size() > args.indexOf("--update") + 1) {
            setupUpdateUI(args.at(args.indexOf("--update") + 1));
        } else if (args.contains("--rollback-file") && args.size() > args.indexOf("--rollback-file") + 1) {
            setupRollbackUI(args.at(args.indexOf("--rollback-file") + 1));
        } else {
            populateBackupList();
        }
    }

private:
    void setupUpdateUI(const QString &zipPath) {
        m_statusLabel->setText("<h2>Updating RJ Launcher...</h2>");
        m_infoLabel->setText("Please wait while the launcher is being updated.");
        m_backupList->setVisible(false);
        m_progressBar->setVisible(true);
        m_rollbackButton->setEnabled(false);
        
        // Run update logic after window shows
        QTimer::singleShot(1000, this, [this, zipPath]() {
            performUpdate(zipPath);
        });
    }

    void setupRollbackUI(const QString &oldFilePath) {
        m_statusLabel->setText("<h2>Rolling Back...</h2>");
        m_infoLabel->setText("Restoring previous executable version.");
        m_backupList->setVisible(false);
        m_progressBar->setVisible(true);
        m_rollbackButton->setEnabled(false);

        QTimer::singleShot(1000, this, [this, oldFilePath]() {
            processRollbackFile(oldFilePath);
        });
    }

    void performUpdate(const QString &zipPath) {
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir rootDir(dataRoot); rootDir.cdUp();
        QString installPath = rootDir.absolutePath();
        QString exePath = QDir(installPath).absoluteFilePath("RJML.exe");

        // 1. Delete current executable (it should be closed)
        if (QFile::exists(exePath)) {
            if (!QFile::remove(exePath)) {
                QMessageBox::critical(this, "Update Error", "Could not remove current executable. Is it still running?");
                return;
            }
        }

        // 2. Extract and Restart
        if (ExtractZipFile(zipPath, installPath)) {
            finishAndRestart(installPath);
        } else {
            QMessageBox::critical(this, "Update Failed", "Failed to extract update package.");
        }
    }

    void processRollbackFile(const QString &selectedOldFile) {
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir rootDir(dataRoot); rootDir.cdUp();
        QString installPath = rootDir.absolutePath();
        QString currentExe = QDir(installPath).absoluteFilePath("RJML.exe");

        // Naming Algorithm: RJML.<build_type_short><buildnumber>.old
        QString prefix = LAUNCHER_BUILD_TYPE_SHORT;
        QString build = QString::number(GetBuildNumber());
        QString oldStorePath = dataRoot + "LauncherUpdater/LauncherSource/old/";
        QDir().mkpath(oldStorePath);
        
        QString backupName = QString("RJML.%1%2.old").arg(prefix, build);
        QString backupPath = oldStorePath + backupName;

        // 1. Move current to old/
        if (QFile::exists(currentExe)) {
            QFile::remove(backupPath); // Overwrite if exists
            QFile::rename(currentExe, backupPath);
        }

        // 2. Move selected back to root
        if (QFile::rename(selectedOldFile, currentExe)) {
            finishAndRestart(installPath);
        } else {
            QMessageBox::critical(this, "Rollback Failed", "Could not restore the selected executable.");
        }
    }

    void finishAndRestart(const QString &path) {
        QProcess::startDetached(QDir(path).absoluteFilePath("RJML.exe"));
        QCoreApplication::quit();
    }

private slots:
    void populateBackupList() {
        m_backupList->clear();
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir oldDir(dataRoot + "LauncherUpdater/LauncherSource/old/"); // Look in the unified backup root
        if (!oldDir.exists()) {
            m_backupList->addItem("No backups found.");
            m_rollbackButton->setEnabled(false);
            return;
        }

        QStringList backups = oldDir.entryList(QStringList() << "RJML.*", QDir::Files, QDir::Time | QDir::Reversed);
        if (backups.isEmpty()) {
            m_backupList->addItem("No backups found.");
            m_rollbackButton->setEnabled(false);
        } else {
            m_backupList->addItems(backups);
            m_rollbackButton->setEnabled(true);
        }
    }

    void performRollback() {
        QListWidgetItem *selectedItem = m_backupList->currentItem();
        if (!selectedItem) {
            QMessageBox::warning(this, "No Selection", "Please select a backup to restore.");
            return;
        }

        QString backupFileName = selectedItem->text();
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QString backupZipPath = QDir(dataRoot + "LauncherUpdater/LauncherSource/old/").absoluteFilePath(backupFileName);
        QDir rootDir(dataRoot);
        rootDir.cdUp();
        QString installPath = rootDir.absolutePath();

        if (QMessageBox::question(this, "Confirm Rollback",
                                  QString("Are you sure you want to restore '%1'? This will overwrite your current installation.").arg(backupFileName),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }

        // 1. Wipe the current installation (except oldlauncher and Tools)
        QDir appDir(installPath);
        if (appDir.exists()) {
            QDirIterator it(installPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            while (it.hasNext()) {
                QString entry = it.next();
                QFileInfo info(entry);
                if (info.fileName() == "RJLData" || info.fileName() == "Tools") continue; // Don't delete user data or tools
                if (info.isDir()) QDir(entry).removeRecursively();
                else QFile::remove(entry);
            }
        }

        // 2. Extract the selected backup
        if (ExtractZipFile(backupZipPath, installPath)) {
            finishAndRestart(installPath);
        } else {
            QMessageBox::critical(this, "Restore Failed", "Failed to extract the backup.");
        }
    }

private:
    QListWidget *m_backupList;
    QPushButton *m_rollbackButton;
    QPushButton *m_cancelButton;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    QString m_backupZipToRestore;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QString backupArg;
    if (argc > 1) backupArg = argv[1];
    MadeChangesDialog w(backupArg);
    w.show();
    return a.exec();
}

#include "MadeChanges.moc"