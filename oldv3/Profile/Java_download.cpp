#include "jvmdownloadernew.h" // Include the new Java downloader
#include <QWidget>

// This file now simply provides the external interface to the new JVMDownloaderNew
void ShowJVMDownloaderNew(QWidget *parent);

void ShowJavaDownloadMenu(QWidget *parent) {
    ShowJVMDownloaderNew(parent);
}