#include <QDir>
#include <QStringList>
#include <QFileInfoList>
#include <QCoreApplication>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include "Core.h"

/**
 * rollback.cpp - Handles detection and restoration of old launcher versions.
 */

QStringList DetectRollbackPackages() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QString rollbackPath = dataRoot + "LauncherUpdater/LauncherSource/old";

    QDir oldDir(rollbackPath);
    if (!oldDir.exists()) {
        oldDir.mkpath(".");
    }

    // Look specifically in the LauncherSource/old folder
    QStringList filters = {"*.old", "*.bak", "RJML.*.zip"};
    return oldDir.entryList(filters, QDir::Files, QDir::Time);
}

void TriggerRollback(const QString &oldFileName) {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    // Target the specific rollback folder directly
    QString oldFilePath = QDir(dataRoot + "LauncherUpdater/LauncherSource/old").absoluteFilePath(oldFileName);

    if (!QFile::exists(oldFilePath)) {
        QMessageBox::critical(nullptr, "Error", "Backup file not found: " + oldFileName);
        return;
    }

    QString appPath = QCoreApplication::applicationFilePath();

#ifdef Q_OS_WIN
    // On Windows, launch MadeChanges.exe to handle the rollback process
    QString madeChangesPath = QCoreApplication::applicationDirPath() + "/Tools/MadeChanges.exe";
    if (QProcess::startDetached(madeChangesPath, {"--rollback-file", oldFilePath})) {
        QCoreApplication::quit();
    }
#else
    // Linux/macOS direct rollback using shell script
    QString program = "/bin/sh";
    QStringList arguments;
    arguments << "-c" << QString("sleep 2 && mv -f \"%1\" \"%2\" && chmod +x \"%2\" && \"%2\" &")
                         .arg(oldFilePath, appPath);
    
    if (QProcess::startDetached(program, arguments)) {
        QCoreApplication::quit();
    }
#endif
}