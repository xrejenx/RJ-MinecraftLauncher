#include <QString>
#include <QDebug>
#include <QWidget>
#include <QDateTime>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>
#endif
#include <QSettings>
#include <QCoreApplication>
#include "Core.h"

/**
 * ConsoleOutput.cpp - Manages logging and debug output
 */

void LogLauncherEvent(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    qDebug() << QString("[%1] [EVENT]: %2").arg(timestamp, message);
}

QString MinecraftLauncher::getRJLDataPath() {
    QString dataRoot;
#ifdef Q_OS_WIN
    dataRoot = "C:/RJLData/";
#else
    dataRoot = QDir::homePath() + "/.RJLData/";
#endif
    QDir().mkpath(dataRoot);
    return dataRoot;
}

void LogWindowContent(QWidget *window) {
    if (window) {
        qDebug() << "[WINDOW DEBUG]: Inspecting " << window->windowTitle();
    } else {
        qDebug() << "[WINDOW DEBUG]: Attempted to log a null window.";
    }
}

void EnsureConsoleVisibility() {
#ifdef Q_OS_WIN
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QSettings settings(dataRoot + "launcher.ini", QSettings::IniFormat);
    bool enabled = settings.value("launcher/showConsole", false).toBool();
    HWND hwnd = GetConsoleWindow();

    if (enabled) {
        if (!hwnd) {
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            hwnd = GetConsoleWindow();
        }
        if (hwnd) ShowWindow(hwnd, SW_SHOW);
    } else {
        if (hwnd) ShowWindow(hwnd, SW_HIDE);
    }
#endif
    LogLauncherEvent("Console visibility checked.");
}