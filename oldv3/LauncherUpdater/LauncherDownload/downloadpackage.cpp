#include <QString>
#include <QDir>

/**
 * downloadpackage.cpp - Downloads both the binary zip and its associated SHA files.
 */
#include "LauncherUpdater/LauncherDownload/download.h" // For DownloadFileToPath

void DownloadFullUpdatePackage(const QString &zipUrl, const QString &sha1Url, const QString &version) {
    QString baseDir = "LauncherUpdater/LauncherSource/";
    QDir().mkpath(baseDir);

    DownloadFileToPath(zipUrl, baseDir + "update-" + version + ".zip");
    if (!sha1Url.isEmpty()) {
        DownloadFileToPath(sha1Url, baseDir + "update-" + version + ".zip.sha1");
    }
}
