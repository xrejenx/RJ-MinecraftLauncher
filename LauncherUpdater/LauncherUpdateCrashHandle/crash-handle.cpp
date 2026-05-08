#include <QString>
#include <QMessageBox>

// Forward declaration to the logging function in crash.cpp
void LogUpdateCrash(const QString &error);

/**
 * crash-handle.cpp - Orchestrates UI feedback and logging when an update fails.
 */
void HandleUpdateCrash(const QString &context, const QString &errorMessage) {
    QString fullError = QString("[%1] %2").arg(context, errorMessage);
    
    // Log to file
    LogUpdateCrash(fullError);
    
    // Notify User
    QMessageBox::critical(nullptr, "Update Process Crashed", "An unexpected error occurred during the update.\n\n" + fullError);
}