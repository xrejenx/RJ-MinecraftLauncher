#include <QString>
#include <QFile>
#include <QTextStream>

/**
 * launchargs_creator.cpp - Generates the default start.txt for instances
 */

void CreateLaunchArgs(const QString &targetDirPath, const QString &versionId) {
    QFile file(targetDirPath + "/args.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Add custom launch arguments for " << versionId << " below (e.g., --width 1280)";
        file.close();
    }
}