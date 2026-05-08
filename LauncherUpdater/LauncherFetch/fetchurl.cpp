#include <QString>

/**
 * fetchurl.cpp - Logic for constructing or resolving specific download URLs.
 */
namespace LauncherFetch {
    QString getZipDownloadUrl(const QString &repoBase, const QString &tag, const QString &osName) {
        // Example format: https://github.com/user/repo/releases/download/build-32/RJML-linux.zip
        QString format = "%1/releases/download/%2/RJML-%3.zip";
        if (osName == "windows") format = "%1/releases/download/%2/RJML-win.zip";
        return format.arg(repoBase, tag, osName);
    }
}