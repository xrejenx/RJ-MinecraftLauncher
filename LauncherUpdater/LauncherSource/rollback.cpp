#include <QDir>
#include <QStringList>
#include <QFileInfoList>
#include <QCoreApplication>
#include <QProcess>
#include <QMessageBox>
#include <QFile>

/**
 * rollback.cpp - Handles detection and restoration of old launcher versions.
 */

QStringList DetectRollbackPackages() {
#ifdef Q_OS_WIN
    QDir oldDir("C:/RJLauncherData/oldlauncher");
    if (!oldDir.exists()) return {};
    return oldDir.entryList({"RJML.*.zip"}, QDir::Files, QDir::Time);
#else
    QDir oldDir("LauncherUpdater/LauncherSource/old");
    if (!oldDir.exists()) return {};
    
    // Find all .old files, sorted by newest first
    return oldDir.entryList({"*.old"}, QDir::Files, QDir::Time);
#endif
}

void TriggerRollback(const QString &oldFileName) {
#ifdef Q_OS_WIN
    QString appPath = QCoreApplication::applicationFilePath();
    // On Windows, launch MadeChanges.exe to handle the rollback
    QString madeChangesPath = "C:/RJLauncherData/Tools/MadeChanges.exe";
    if (QFile::exists(madeChangesPath)) {
        if (QProcess::startDetached(madeChangesPath, {oldFileName})) {
            QCoreApplication::quit();
        } else {
            QMessageBox::critical(nullptr, "Rollback Error", "Failed to launch the rollback tool.");
        }
    } else {
        QMessageBox::critical(nullptr, "Rollback Error", "Rollback tool (MadeChanges.exe) not found at " + madeChangesPath);
    }
#else // Linux/macOS, use existing in-place rollback logic
    QString oldFilePath = QDir::current().absoluteFilePath("LauncherUpdater/LauncherSource/old/" + oldFileName);
    QString appPath = QCoreApplication::applicationFilePath();
    // ... (existing Linux/macOS rollback logic remains)
    // This part of the diff is intentionally left out as it's not changing for non-Windows.
#endif
}