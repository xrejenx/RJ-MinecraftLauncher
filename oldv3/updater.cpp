#include <QMessageBox>
#include <QString>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include "Core.h"
#include "LauncherUpdater/LauncherDownload/downloadzip.h"

extern int GetBuildNumber(); // From version.cpp
extern QString GetBuildTypePrefix(); // From version.cpp
extern QString GetBuildTypePrefixShort(); // From LauncherFetch/fetch.cpp

/**
 * updater.cpp - Handles the final user confirmation and state management for updates.
 */
bool PromptForUpdateInstallation(const QString &version) {
    auto reply = QMessageBox::question(nullptr, "Install Update", 
        QString("Version %1 has been downloaded. Would you like to restart and update now?").arg(version),
        QMessageBox::Yes | QMessageBox::No);
    
    return (reply == QMessageBox::Yes);
}

void TriggerLocalUpdate(const QString &localZipName) {
    if (!PromptForUpdateInstallation(localZipName)) return;
    QString dataRoot = MinecraftLauncher::getRJLDataPath();

#ifdef Q_OS_WIN
    QString pkgPath = dataRoot + "LauncherUpdater/LauncherSource/" + localZipName;
    if (localZipName.toLower().endsWith(".exe")) {
        if (QProcess::startDetached(pkgPath)) {
            QCoreApplication::quit();
        } else {
            QMessageBox::critical(nullptr, "Update Error", "Failed to launch the update installer: " + localZipName);
        }
    } else if (localZipName.toLower().endsWith(".zip")) {
        QString tool = QCoreApplication::applicationDirPath() + "/Tools/MadeChanges.exe";
        if (QFile::exists(tool)) {
            if (QProcess::startDetached(tool, {"--update", pkgPath})) {
                QCoreApplication::quit();
            } else {
                QMessageBox::critical(nullptr, "Update Error", "Failed to start updater tool.");
            }
        } else {
            QMessageBox::critical(nullptr, "Update Error", "Updater tool (MadeChanges.exe) not found.");
        }
    }
#else
    // For Linux/macOS, continue with the existing in-place update logic
    QString zipPath = dataRoot + "LauncherUpdater/LauncherSource/" + localZipName;
    QString extractDirRoot = dataRoot + "LauncherUpdater/LauncherSource/ExtractedUpdate/";
    
    QDir(extractDirRoot).removeRecursively();
    QDir().mkpath(extractDirRoot);

    if (ExtractZipFile(zipPath, extractDirRoot)) {
        QString appPath = QCoreApplication::applicationFilePath();
        QString appDir = QCoreApplication::applicationDirPath();
        QString extractDir = extractDirRoot;

        QDirIterator it(extractDirRoot, QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFile::remove(it.next());
        }
        
        int currentBuild = GetBuildNumber();
        QString shortPrefix = GetBuildTypePrefixShort().toLower();
        QString backupName = QString("RJML%1%2.exe.old").arg(shortPrefix).arg(currentBuild);
        QString backupPath = dataRoot + "LauncherUpdater/LauncherSource/old/" + backupName;
        QDir().mkpath(dataRoot + "LauncherUpdater/LauncherSource/old/");

        QDir checkDir(extractDir);
        QStringList entries = checkDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        if (entries.size() == 1 && QFileInfo(extractDir + "/" + entries[0]).isDir()) {
            extractDir = QDir(extractDir + "/" + entries[0]).absolutePath();
        }
        
        QString program = "/bin/sh";
        QStringList arguments;
        arguments << "-c" << QString("sleep 2 && mv -f \"%1\" \"%4\" && cp -rf \"%2/.\" \"%3/\" && chmod +x \"%1\" && \"%1\" &")
            .arg(appPath, extractDir, appDir, backupPath);
        
        if (QProcess::startDetached(program, arguments)) {
            QCoreApplication::quit();
        } else {
            QMessageBox::critical(nullptr, "Update Error", "Failed to start the update handover process.");
        }
    }
#endif
