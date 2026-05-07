#include <QString>
#include <QDebug>
#include <QWidget>
#include <QDateTime>

/**
 * ConsoleOutput.cpp - Manages logging and debug output
 */

void LogLauncherEvent(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    qDebug() << QString("[%1] [EVENT]: %2").arg(timestamp, message);
}

void LogWindowContent(QWidget *window) {
    if (window) {
        qDebug() << "[WINDOW DEBUG]: Inspecting " << window->windowTitle();
    } else {
        qDebug() << "[WINDOW DEBUG]: Attempted to log a null window.";
    }
}

void EnsureConsoleVisibility() {
    // For now, we log the check. This can later be expanded to toggle a separate debug window.
    LogLauncherEvent("Console visibility checked.");
}