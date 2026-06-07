#include <QDir>
#include <QStringList>
#include <QFileInfoList>
#include "Core.h"

QStringList DetectDownloadedPackages() {
    QDir sourceDir(MinecraftLauncher::getRJLDataPath() + "LauncherUpdater/LauncherSource");
    if (!sourceDir.exists()) sourceDir.mkpath(".");

    QStringList packages;
    QStringList filters;

    filters << "*.zip" << "*.tar.gz" << "*.exe";

    QFileInfoList list = sourceDir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &info : list) {
        packages << info.fileName();
    }

    return packages;
}  // <-- this was missing
