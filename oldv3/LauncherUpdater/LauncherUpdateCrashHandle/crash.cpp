#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QDir>

void LogUpdateCrash(const QString &error) {
    QDir().mkpath("LauncherUpdater/log/crash");
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString logPath = QString("LauncherUpdater/log/crash/log-%1.log").arg(timestamp);
    
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Crash Report - " << QDateTime::currentDateTime().toString() << "\n";
        out << "Error: " << error << "\n";
    }
}