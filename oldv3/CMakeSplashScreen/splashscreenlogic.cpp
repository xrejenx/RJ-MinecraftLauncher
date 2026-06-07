#include "splash.h"
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>

namespace SplashAssetManager { QPixmap getSplashLogo(); }
QColor GetLightestDominantColor(const QPixmap &pixmap);
int GetBuildNumber();

struct ReleaseAsset {
    QString name;
    QString url;
    qint64 size;
};

struct ReleaseInfo {
    int buildNumber;
    QString tagName;
    QString description;
    bool isPreRelease;
    QList<ReleaseAsset> assets;
};

struct UpdateInfo {
    QList<ReleaseInfo> allReleases;
};

UpdateInfo CheckForUpdates();
bool ProcessZipUpdate(const QString &url, const QString &version); // Forward declaration
#include "LauncherUpdater/LauncherDownload/downloadzip.h" // For ExtractZipFile

class UpdateSelectionDialog : public QDialog {
public:
    UpdateSelectionDialog(const UpdateInfo &info, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Launcher Update Available");
        setFixedSize(400, 250);
        QVBoxLayout *layout = new QVBoxLayout(this);

        QTabWidget *tabs = new QTabWidget(this);

        // Tab 1: Auto Detect (Find newest)
        QWidget *autoTab = new QWidget();
        QVBoxLayout *autoLayout = new QVBoxLayout(autoTab);
        
        const ReleaseInfo &best = info.allReleases.first();
        QString bestUrl;
        // Pick the most appropriate binary for the current platform
        for(const auto &asset : best.assets) {
            bestUrl = asset.url;
#ifdef Q_OS_WIN
            if(asset.name.contains("win", Qt::CaseInsensitive)) break;
#else
            if(asset.name.contains("linux", Qt::CaseInsensitive)) break;
#endif
        }

        autoLayout->addWidget(new QLabel(QString("Recommended Version: <b>%1</b>").arg(best.tagName)));
        autoLayout->addWidget(new QLabel(QString("Type: %1").arg(best.isPreRelease ? "Pre-release" : "Stable Release")));
        
        QPushButton *btnAuto = new QPushButton(QString("Update to %1").arg(best.tagName), this);
        connect(btnAuto, &QPushButton::clicked, [this, bestUrl, best]() {
            selectedUrl = bestUrl;
            selectedVersion = best.buildNumber;
            accept();
        });
        autoLayout->addStretch();
        autoLayout->addWidget(btnAuto);
        tabs->addTab(autoTab, "Auto Detect");

        // Tab 2: Custom
        QWidget *customTab = new QWidget();
        QVBoxLayout *customLayout = new QVBoxLayout(customTab);
        
        for (const auto &rel : info.allReleases) {
            QPushButton *btn = new QPushButton(QString("%1 (%2)").arg(rel.tagName, rel.isPreRelease ? "Pre" : "Stable"), this);
            connect(btn, &QPushButton::clicked, [this, rel]() {
                selectedUrl = rel.assets.isEmpty() ? "" : rel.assets.first().url;
                selectedVersion = rel.buildNumber;
                accept();
            });
            customLayout->addWidget(btn);
        }
        
        if (info.allReleases.isEmpty()) {
            customLayout->addWidget(new QLabel("No updates found in custom list."));
        }

        customLayout->addStretch();
        tabs->addTab(customTab, "Custom");

        layout->addWidget(tabs);

        QHBoxLayout *bottom = new QHBoxLayout();
        QPushButton *btnLater = new QPushButton("Later", this);
        connect(btnLater, &QPushButton::clicked, this, &QDialog::reject);
        bottom->addStretch();
        bottom->addWidget(btnLater);
        layout->addLayout(bottom);
    }

    QString selectedUrl;
    int selectedVersion = 0;
};

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
        splash.setProgress(40);

        // Only prompt for update on launch if a newer version exists
        if (!info.allReleases.isEmpty() && info.allReleases.first().buildNumber > GetBuildNumber()) {
            UpdateSelectionDialog diag(info, &splash);
            if (diag.exec() == QDialog::Accepted) {
                splash.setTaskText(QString("Task: Downloading version %1...").arg(diag.selectedVersion));
                ProcessZipUpdate(diag.selectedUrl, QString::number(diag.selectedVersion));
                return false;
            }
        } else {
            splash.setTaskText("Task: No new updates found. Skipping...");
            QApplication::processEvents();
            QThread::msleep(1000); // Give user time to read the status
        }
        
        splash.setProgress(50);
        splash.setTaskText("Task: Checking environment...");
        QApplication::processEvents();
        
        // Additional checks (Folders, Java etc) would go here
        
        splash.setProgress(100);
        splash.setTaskText("Task: Done. Starting Launcher...");
        
        QTimer::singleShot(3000, &splash, &QDialog::accept);
        if (splash.exec() == QDialog::Accepted) {
            return true;
        }
        
        return false;
    }
}
