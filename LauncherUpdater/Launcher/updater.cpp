#include <QMessageBox>
#include <QString>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include "LauncherUpdater/LauncherDownload/downloadzip.h"

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
    if (PromptForUpdateInstallation(localZipName)) {
        QString zipPath = QDir::current().absoluteFilePath("LauncherUpdater/LauncherSource/" + localZipName);
        QString extractDirRoot = QDir::current().absoluteFilePath("LauncherUpdater/LauncherSource/ExtractedUpdate/");
        
        // Clean and prepare extraction directory
        QDir(extractDirRoot).removeRecursively();
        QDir().mkpath(extractDirRoot);

        if (ExtractZipFile(zipPath, extractDirRoot)) {
            QString appPath = QCoreApplication::applicationFilePath();
            QString appDir = QCoreApplication::applicationDirPath();
            QString extractDir = extractDirRoot;

            // Handle nested folders (e.g. ZIP contains RJML-v1/binary instead of just binary)
            QDir checkDir(extractDir);
            QStringList entries = checkDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
            if (entries.size() == 1 && QFileInfo(extractDir + "/" + entries[0]).isDir()) {
                extractDir = QDir(extractDir + "/" + entries[0]).absolutePath();
            }
            
            QString program;
            QStringList arguments;

#ifdef Q_OS_WIN
            program = "cmd.exe";
            // Use xcopy /i to handle directory creation if needed
            arguments << "/c" << QString("timeout /t 2 > nul && move /y \"%1\" \"%1.bak\" && xcopy /s /e /y /q /i \"%2\\*\" \"%3\" && start \"\" \"%1\"")
                .arg(QDir::toNativeSeparators(appPath), 
                     QDir::toNativeSeparators(extractDir), 
                     QDir::toNativeSeparators(appDir));
#elif defined(Q_OS_MAC)
            program = "/bin/sh";
            // Unlink original binary, copy contents, set permissions, and open
            arguments << "-c" << QString("sleep 2 && mv -f \"%1\" \"%1.old\" && cp -Rf \"%2/.\" \"%3/\" && chmod +x \"%1\" && open \"%1\"")
                .arg(appPath, extractDir, appDir);
#else // Linux (and others)
            program = "/bin/sh";
            // Unlink original binary, copy contents (merging root), set permissions, and relaunch
            arguments << "-c" << QString("sleep 2 && mv -f \"%1\" \"%1.old\" && cp -rf \"%2/.\" \"%3/\" && chmod +x \"%1\" && \"%1\" &")
                .arg(appPath, extractDir, appDir);
#endif
            
            if (QProcess::startDetached(program, arguments)) {
                QCoreApplication::quit();
            } else {
                QMessageBox::critical(nullptr, "Update Error", "Failed to start the update handover process.");
            }
        }
    }
}