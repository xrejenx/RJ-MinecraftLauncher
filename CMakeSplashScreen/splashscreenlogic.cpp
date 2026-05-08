#include "splash.h"
#include <QApplication>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>

namespace SplashAssetManager { QPixmap getSplashLogo(); }
QColor GetLightestDominantColor(const QPixmap &pixmap);

struct UpdateInfo {
    bool releaseAvailable;
    int releaseBuild;
    QString releaseUrl;
    bool preReleaseAvailable;
    int preReleaseBuild;
    QString preReleaseUrl;
};

UpdateInfo CheckForUpdates();
void DownloadAndInstallUpdate(const QString &url, int version);

namespace SplashScreenLogic {
    bool run() {
        SplashScreen splash;
        QPixmap logo = SplashAssetManager::getSplashLogo();
        splash.setLogo(logo);
        splash.setBgColor(GetLightestDominantColor(logo));
        splash.show();
        
        splash.setTaskText("Task: Fetching update...");
        splash.setProgress(10);
        QApplication::processEvents();
        
        UpdateInfo info = CheckForUpdates();
        if (info.releaseAvailable || info.preReleaseAvailable) {
            QMessageBox msgBox(&splash);
            msgBox.setWindowTitle("Update Found");
            msgBox.setText("A new version of the launcher is available. Would you like to update?");
            
            QPushButton *stableBtn = nullptr;
            QPushButton *preBtn = nullptr;
            QPushButton *laterBtn = msgBox.addButton("Later", QMessageBox::RejectRole);

            if (info.releaseAvailable)
                stableBtn = msgBox.addButton(QString("Install Release (%1)").arg(info.releaseBuild), QMessageBox::AcceptRole);
            if (info.preReleaseAvailable)
                preBtn = msgBox.addButton(QString("Install Pre-release (%1)").arg(info.preReleaseBuild), QMessageBox::AcceptRole);

            msgBox.exec();

            if (msgBox.clickedButton() == stableBtn) {
                splash.setTaskText(QString("Task: Downloading release %1...").arg(info.releaseBuild));
                DownloadAndInstallUpdate(info.releaseUrl, info.releaseBuild);
                return false;
            } else if (msgBox.clickedButton() == preBtn) {
                splash.setTaskText(QString("Task: Downloading pre-release %1...").arg(info.preReleaseBuild));
                DownloadAndInstallUpdate(info.preReleaseUrl, info.preReleaseBuild);
                return false;
            }
        }
        
        splash.setProgress(50);
        splash.setTaskText("Task: Checking environment...");
        QApplication::processEvents();
        
        // Additional checks (Folders, Java etc) would go here
        
        splash.setProgress(100);
        splash.setTaskText("Task: Done");
        
        QTimer::singleShot(500, &splash, &QDialog::accept);
        if (splash.exec() == QDialog::Accepted) {
            return true;
        }
        
        return false;
    }
}