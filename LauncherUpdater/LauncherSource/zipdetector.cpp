#include <QDir>
#include <QStringList>
#include <QFileInfoList>

QStringList DetectDownloadedPackages() {
    QDir sourceDir("LauncherUpdater/LauncherSource");
    if (!sourceDir.exists()) sourceDir.mkpath(".");
    
    QStringList packages;
    QFileInfoList list = sourceDir.entryInfoList(QStringList() << "*.zip" << "*.tar.gz", QDir::Files);
    
    for (const QFileInfo &info : list) {
        packages << info.fileName();
    }
    
    return packages;
}