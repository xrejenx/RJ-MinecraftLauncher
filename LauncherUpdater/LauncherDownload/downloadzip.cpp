#include "downloadzip.h"
#include "download.h"
#include <QString>
#include <QProcess>
#include <QDir>
#include <QMessageBox>

/**
 * External dependencies (from ConsoleOutput.cpp)
 */
void LogLauncherEvent(const QString &message);

/**
 * ExtractZipFile - Pure extraction logic for local files.
 */
bool ExtractZipFile(const QString &zipFilePath, const QString &destinationPath) {
    QDir().mkpath(destinationPath);
    
    // Convert to native Windows separators for external tools
    QString nativeZip = QDir::toNativeSeparators(zipFilePath);
    QString nativeDest = QDir::toNativeSeparators(destinationPath);

    QString extractorPath;
#ifdef Q_OS_WIN
    extractorPath = QDir::current().absoluteFilePath("Lib/7za.exe");
#else
    extractorPath = QDir::current().absoluteFilePath("Lib/7za");
    // Ensure execution permissions on Linux
    QFile::setPermissions(extractorPath, QFile::permissions(extractorPath) | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
#endif

    QProcess extractProc;
    if (QFile::exists(extractorPath)) {
        // Use 7zip from Lib folder: x (extract), -o (output), -y (yes to all / overwrite)
        extractProc.start(extractorPath, {"x", nativeZip, "-o" + nativeDest, "-y"});
        extractProc.waitForFinished(-1); 
    } else {
        // Fallback for Installer: Use native OS commands if 7zip lib isn't present yet
#ifdef Q_OS_WIN
        extractProc.start("powershell", {"-NoProfile", "-Command", QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(nativeZip, nativeDest)});
#else
        extractProc.start("unzip", {"-o", nativeZip, "-d", nativeDest});
#endif
        extractProc.waitForFinished(-1);
    }
    
    if (extractProc.exitStatus() == QProcess::NormalExit) {
        LogLauncherEvent("Successfully extracted " + zipFilePath);
        return true;
    } else {
        QString error = QString("Extraction failed for %1").arg(zipFilePath);
        LogLauncherEvent(error);
        QMessageBox::critical(nullptr, "Extraction Error", error);
        return false;
    }
}

/**
 * downloadzip.cpp - High-level handler for downloading a zip and immediately extracting it.
 */
bool ProcessZipUpdate(const QString &url, const QString &version) {
    QString zipName = QString("LauncherUpdater/LauncherDownload/update-%1.zip").arg(version);
    QString extractPath = "LauncherUpdater/LauncherSource/ExtractedZip/";

    if (!DownloadFileToPath(url, zipName)) return false;

    return ExtractZipFile(zipName, extractPath);
}