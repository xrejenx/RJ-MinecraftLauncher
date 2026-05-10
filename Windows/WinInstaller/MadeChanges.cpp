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
QString GetMadeChangesTitle(); // From version.cpp

// Helper to move contents from an extraction folder to a destination, flattening if a single subfolder exists
void moveAndFlatten(const QString &tempExtractPath, const QString &destPath) {
    QDir extractDir(tempExtractPath);
    QStringList entries = extractDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    
    QString sourcePath = tempExtractPath;
    if (entries.size() == 1 && QFileInfo(extractDir.absoluteFilePath(entries[0])).isDir()) {
        sourcePath = extractDir.absoluteFilePath(entries[0]);
    }

    QDir sourceDir(sourcePath);
    for (const QString &f : sourceDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden)) {
        QString oldPath = sourceDir.absoluteFilePath(f);
        QString newPath = QDir(destPath).absoluteFilePath(f);
        if (QFile::exists(newPath)) {
            if (QFileInfo(newPath).isDir()) QDir(newPath).removeRecursively();
            else QFile::remove(newPath);
        }
        QFile::rename(oldPath, newPath);
    }
}

// Fallback if the build system doesn't provide the build type macro
#ifndef LAUNCHER_BUILD_TYPE_SHORT
#define LAUNCHER_BUILD_TYPE_SHORT "r"
#endif

class MadeChangesDialog : public QDialog {
    Q_OBJECT
public:
    explicit MadeChangesDialog(const QString &backupZipToRestore = "", QWidget *parent = nullptr)
        : QDialog(parent), m_backupZipToRestore(backupZipToRestore) {
        setWindowTitle(GetMadeChangesTitle());
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

        // Backup current executable before performing the update
        QString currentExe = QDir(installPath).absoluteFilePath("RJML.exe");
        if (QFile::exists(currentExe)) {
            QString oldStorePath = dataRoot + "LauncherUpdater/LauncherSource/old/";
            QDir().mkpath(oldStorePath);

            QString prefix = LAUNCHER_BUILD_TYPE_SHORT;
            QString build = QString::number(GetBuildNumber());
            QString backupName = QString("RJML.exe.%1%2.old").arg(prefix, build);
            QString backupPath = oldStorePath + backupName;

            QFile::remove(backupPath); // Ensure we can overwrite if an identical build was backed up
            QFile::rename(currentExe, backupPath);
        }

        QString tempExtract = dataRoot + "LauncherUpdater/temp_update/";
        QDir(tempExtract).removeRecursively();
        QDir().mkpath(tempExtract);

        // 2. Extract and Restart
        if (ExtractZipFile(zipPath, tempExtract)) {
            moveAndFlatten(tempExtract, installPath);
            QDir(tempExtract).removeRecursively();
            finishAndRestart(installPath);
        } else {
            QDir(tempExtract).removeRecursively();
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
        
        QString backupName = QString("RJML.exe.%1%2.old").arg(prefix, build);
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

        QString tempExtract = dataRoot + "LauncherUpdater/temp_rollback/";
        QDir(tempExtract).removeRecursively();
        QDir().mkpath(tempExtract);

        // 1. Wipe the current installation (except oldlauncher and Tools)
        QDir appDir(installPath);
        if (appDir.exists()) {
            QDirIterator it(installPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            while (it.hasNext()) {
                QString entry = it.next();
                QFileInfo info(entry);
                if (info.fileName() == "RJLData") continue; 
                if (info.fileName() == "Tools") {
                    // Allow updating tools: delete everything in Tools except the running MadeChanges.exe
                    QDir toolsDir(entry);
                    for (const QString &f : toolsDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
                        if (f.contains("MadeChanges", Qt::CaseInsensitive)) continue;
                        QString fPath = toolsDir.absoluteFilePath(f);
                        if (QFileInfo(fPath).isDir()) QDir(fPath).removeRecursively();
                        else QFile::remove(fPath);
                    }
                    continue;
                }
                if (info.isDir()) QDir(entry).removeRecursively();
                else QFile::remove(entry);
            }
        }

        // 2. Extract the selected backup
        if (ExtractZipFile(backupZipPath, tempExtract)) {
            moveAndFlatten(tempExtract, installPath);
            QDir(tempExtract).removeRecursively();
            finishAndRestart(installPath);
        } else {
            QDir(tempExtract).removeRecursively();
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