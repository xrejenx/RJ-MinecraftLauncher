#include <QDir>
#include <QStringList>
#include <QFileInfoList>

QStringList DetectDownloadedPackages() {
    QDir sourceDir("LauncherUpdater/LauncherSource");
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