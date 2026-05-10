#include <QDir>
#include <QStringList>
#include <QFileInfoList>
#include "Core.h"

QStringList DetectDownloadedPackages() {
    QDir sourceDir(MinecraftLauncher::getRJLDataPath() + "LauncherUpdater/LauncherSource");
    if (!sourceDir.exists()) sourceDir.mkpath(".");
    
    QStringList packages;
    QStringList filters;

#ifdef Q_OS_WIN
    filters << "*.exe";
#else
    filters << "*.zip" << "*.tar.gz";
#endif

    QFileInfoList list = sourceDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo &info : list) {
        packages << info.fileName();
    }
    
    return packages;
}