#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPalette>
#include <QLabel>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QDialog>
#include <QWidget>
#include <QMessageBox>
#include <QTabWidget>
#include <QJsonDocument>
#include <QCloseEvent>
#include <QJsonObject>
#include <QScrollArea>
#include <QSplitter>
#include <QTextBrowser>
#include <QFile>
#include <QDir>
#include <QFileInfo> // Ensure QFileInfo is included
#include <QTimer>
#include <QSettings>
#include <functional>
#include "webview.h"
#include "icon.h"
#include <QProcess>
#include "theme.h"
#include "Core.h"

// Forward declarations for Splash Screen Logic
namespace SplashScreenLogic {
    bool run();
}

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

UpdateInfo CheckForUpdates(); // Now in LauncherUpdater/LauncherFetch/fetch.cpp
void LogLauncherEvent(const QString &message);

QDialog* CreateDownloadProgressDialog(const QList<QPair<QString, QString>>& filesToDownload, QWidget *parent);
void ConnectDownloadDialogSignals(QDialog *dialog, QObject *receiver, 
                                  std::function<void(bool, const QList<QString>&)> finishedCb, 
                                  std::function<void(const QList<QString>&)> cancelledCb);
void TriggerLocalUpdate(const QString &localZipName); // From updater.cpp

#include "LauncherUpdater/LauncherDownload/downloadzip.h" // For ExtractZipFile

// External function interfaces
void ShowJavaProfileWindow(QWidget *parent);
void ShowProfileWindow(QWidget *parent);
void ShowSettingsWindow(QWidget *parent);
void EnsureConsoleVisibility();
QString GetLauncherTitle(); // From LauncherPatch/version.cpp
int GetBuildNumber();      // From LauncherPatch/version.cpp
QString GetAppName();      // From LauncherPatch/version.cpp
QString GetLatestUpdateNote();
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0); // From instance-javaRequirement.cpp
QWidget* CreateModernUpdateTab(QWidget *parent); // From updatecore.cpp
QString GetLwglVersionForMc(const QString &mcVersion); // From lwjglhandle.cpp
void SyncExtractionLibrary(); // From downloadlib.cpp
void PopulateInstanceList(QComboBox *comboBox); // Changed from QListWidget
QString GetLwglNativesPath(const QString &mcVersion); // Forward declaration for LWJGL path (from Profile_java/lwjgl-lib.cpp)

/**
 * Core.cpp - RJ Launcher (Base)
 * 
 * This file provides the entry point and UI layout for the launcher.
 */

MinecraftLauncher::MinecraftLauncher(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(GetLauncherTitle());
    setFixedSize(850, 600);
    
    // Define Data Root for Windows vs other platforms
#ifdef Q_OS_WIN
    QString dataRoot = "C:/RJLauncherData/";
#else
    QString dataRoot = "";
#endif

    // Ensure important folders exist on launch in the data root
    QDir().mkpath(dataRoot + "javas");
    QDir().mkpath(dataRoot + "Lib");
    QDir().mkpath(dataRoot + "Instances");
    QDir().mkpath(dataRoot + "userdata");
    QDir().mkpath(dataRoot + "LauncherUpdater/LauncherSource/old");
    QDir().mkpath(dataRoot + "Assets");
    QDir().mkpath(dataRoot + "icons");
    
    // Initialize Theme System
    ThemeLoader::initialize();
    ApplyLauncherIcon(this);
    SyncExtractionLibrary();

    LogLauncherEvent("Launcher Core Initialized.");
    EnsureConsoleVisibility();

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Greet Banner
    QLabel *warningBanner = new QLabel("Welcome to " + GetAppName(), this);
    warningBanner->setStyleSheet("background-color: rgb(77, 77, 77); color: rgb(194, 194, 194); border: 1px solid rgb(255, 255, 255); padding: 5px; font-size: 11px;");
    warningBanner->setAlignment(Qt::AlignCenter);
    warningBanner->setFixedHeight(30);
    mainLayout->addWidget(warningBanner);

    // Tabs area
    tabs = new QTabWidget(this);
    tabs->addTab(new QLabel("News content goes here...", this), "Update Notes");
    gameConsole = new QPlainTextEdit(this);
    gameConsole->setReadOnly(true);
    gameConsole->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: 'Courier New', monospace;");
    tabs->addTab(gameConsole, "Game Output");
    
    // --- Tab 3: Update Manager (Split View) ---
    QWidget *updateManagerTab = CreateModernUpdateTab(this); // Now from updatecore.cpp
    tabs->addTab(updateManagerTab, "Launcher Update");

    // --- Tab 4: Theme ---
    QWidget *themeTab = new QWidget(this);
    QVBoxLayout *themeTabLayout = new QVBoxLayout(themeTab);
    themeTabLayout->setAlignment(Qt::AlignTop);

    QHBoxLayout *themeSelectLayout = new QHBoxLayout();
    QComboBox *themeSelector = new QComboBox(this);
    themeSelector->addItems(ThemeLoader::getAvailableThemes());
    themeSelector->setCurrentText(ThemeLoader::getSelectedTheme());

    QPushButton *createThemeBtn = new QPushButton("Create", this);
    themeSelectLayout->addWidget(new QLabel("Select Theme:"));
    themeSelectLayout->addWidget(themeSelector, 1);
    themeSelectLayout->addWidget(createThemeBtn);
    themeTabLayout->addLayout(themeSelectLayout);

    QLabel *previewLabel = new QLabel("Theme Preview", this);
    previewLabel->setFixedSize(400, 225);
    previewLabel->setStyleSheet("border: 2px solid gray;");
    previewLabel->setAlignment(Qt::AlignCenter);
    themeTabLayout->addWidget(previewLabel, 0, Qt::AlignCenter);

    QPushButton *applyThemeBtn = new QPushButton("Apply Theme", this);
    themeTabLayout->addStretch();
    themeTabLayout->addWidget(applyThemeBtn, 0, Qt::AlignRight);

    tabs->addTab(themeTab, "Theme");

    mainLayout->addWidget(tabs, 1);

    auto updatePreview = [previewLabel, themeSelector]() {
        QString imgPath = ThemeLoader::getThemeImagePath(themeSelector->currentText());
        if (!imgPath.isEmpty() && QFile::exists(imgPath)) {
            QPixmap pix(imgPath);
            previewLabel->setPixmap(pix.scaled(previewLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            previewLabel->setText("Solid Color Theme\n(No Image)");
        }
    };

    connect(themeSelector, &QComboBox::currentTextChanged, this, updatePreview);
    connect(applyThemeBtn, &QPushButton::clicked, this, [themeSelector]() {
        ThemeLoader::setSelectedTheme(themeSelector->currentText());
        ThemeLoader::applyTheme();
    });
    connect(createThemeBtn, &QPushButton::clicked, this, [this, themeSelector]() {
        ShowCreateThemeDialog(this, [themeSelector]() {
            themeSelector->clear();
            themeSelector->addItems(ThemeLoader::getAvailableThemes());
        });
    });

    updatePreview();

    // Bottom Controls Layout
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    // Profile Section
    QVBoxLayout *profileSectionLayout = new QVBoxLayout(); 
    QLabel *profileLabel = new QLabel("Profile:", this);
    profileComboBox = new QComboBox(this);
    profileComboBox->setMinimumWidth(180);
    PopulateInstanceList(profileComboBox);
    
    QHBoxLayout *profileButtonsLayout = new QHBoxLayout();
    QPushButton *newProfile = new QPushButton("New Profile", this);
    QPushButton *editProfile = new QPushButton("Edit Profile", this);
    profileButtonsLayout->addWidget(newProfile);
    profileButtonsLayout->addWidget(editProfile);

    profileSectionLayout->addWidget(profileLabel);
    profileSectionLayout->addWidget(profileComboBox);
    profileSectionLayout->addLayout(profileButtonsLayout);

    connect(newProfile, &QPushButton::clicked, this, [this]() {
        ShowJavaProfileWindow(this);
        PopulateInstanceList(profileComboBox); // Refresh list after dialog closes
        updateUserLabel();
    });
    connect(editProfile, &QPushButton::clicked, this, [this]() {
        ShowJavaProfileWindow(this);
        PopulateInstanceList(profileComboBox); // Refresh list after dialog closes
        updateUserLabel();
    });

    // mcProcess is already a member, no need to redeclare
    mcProcess = new QProcess(this);

    // Play Button (Centered Large Button)
    playBtn = new QPushButton("PLAY", this);
    playBtn->setFixedSize(200, 60);
    // Setting font via QFont instead of CSS to preserve native OS button textures
    QFont playFont = playBtn->font();
    playFont.setBold(true);
    playFont.setPointSize(18);
    playBtn->setFont(playFont);

    connect(playBtn, &QPushButton::clicked, this, [this]() {
        tabs->setCurrentIndex(1); // Auto-navigate to Game Output tab
        gameConsole->clear();

        if (mcProcess->state() != QProcess::NotRunning) {
            mcProcess->terminate();
            if (!mcProcess->waitForFinished(3000)) mcProcess->kill();
            playBtn->setText("PLAY");
            return;
        }

        QString selectedInstanceName = profileComboBox->currentText(); // Get selected text from QComboBox
        if (selectedInstanceName.isEmpty()) { // Check if anything is selected
            QMessageBox::warning(this, "Launch Error", "Please select an instance from the list.");
            return;
        }

        QSettings settings(QCoreApplication::applicationDirPath() + "/launcher.ini", QSettings::IniFormat);
        
        QFile file("LauncherSession.json");
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Launch Error", "No active session found. Please log in using 'Switch User'.");
            return;
        }

        QJsonObject session = QJsonDocument::fromJson(file.readAll()).object();
        file.close();

        QString instanceName = selectedInstanceName; // Use the selected text directly
        QString instDir = QDir::current().absoluteFilePath("Instances/" + instanceName);
        QString absoluteGameDir = instDir;
        
        QJsonObject instData;
        QFile instInfo(instDir + "/instance.json");
        if (instInfo.open(QIODevice::ReadOnly)) {
            instData = QJsonDocument::fromJson(instInfo.readAll()).object();
            instInfo.close();
        }

        QString argsPath = instDir + "/args.txt";
        QString launchArgs;
        QFile argsFile(argsPath);
        if (!argsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Create an empty args.txt if missing to avoid launch failure
            launchArgs = "";
        }
        else {
            launchArgs = argsFile.readAll();
            argsFile.close();
        }

        // Handle potential naming variations in session data
        QString username = session.contains("username") ? session["username"].toString() : session["Username"].toString();
        QString uuid = session["uuid"].toString();

        QString version = instData["version"].toString();
        QString mainClass = instData["mainClass"].toString();
        QString jarName = QString("Minecraft %1.jar").arg(version);
        
        // Determine asset index (usually major.minor, e.g., 1.21.1 -> 1.21) - This is a placeholder, needs actual parsing
        QStringList verParts = version.split('.');
        QString assetIndex = instData.contains("assetIndex") ? instData["assetIndex"].toString() : 
                             ((verParts.size() >= 2) ? (verParts[0] + "." + verParts[1]) : version);
        
        QString javaPath = settings.value("java/path", "java").toString();

        // Try to find the specific Java required for this version in our 'javas' folder
        int reqMajor = GetRequiredJavaMajorVersion(version, instData["javaMajor"].toInt());
        QDir javasDir("javas");
        QStringList candidates = javasDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dir : candidates) {
            // Look for folders like zulu-jdk-25 or jdk-17
            if (dir.contains(QString::number(reqMajor))) {
                QString binName = "java";
#ifdef Q_OS_WIN
                binName = "java.exe";
#endif
                QString detectedPath = QDir::current().absoluteFilePath("javas/" + dir + "/bin/" + binName);
                if (QFile::exists(detectedPath)) {
                    javaPath = detectedPath;
                    break;
                }
            }
        }

        int ram = settings.value("java/ram", 2).toInt();

        // Native path and classpath
        QString cpSeparator = ";"; // Windows
#ifndef Q_OS_WIN
        cpSeparator = ":"; // Linux
#endif

        QString lwglVer = GetLwglVersionForMc(version);
        QString absoluteLwglDir = QDir::current().absoluteFilePath("Lib/lwjgl-" + lwglVer);
        QString absoluteNativesPath = GetLwglNativesPath(version);

        // Use absolute paths for the classpath to ensure JARs are found
        QString absoluteJarPath = QDir(instDir).absoluteFilePath(jarName);
        QString classpath = QString("%1%2%3/*").arg(absoluteJarPath, cpSeparator, absoluteLwglDir);

        QStringList processArgs;
        processArgs << QString("-Xmx%1G").arg(ram);
        processArgs << QString("-Djava.library.path=%1").arg(absoluteNativesPath);
        processArgs << "-cp" << classpath;
        processArgs << mainClass;

        // Add Core Minecraft Arguments
        processArgs << "--username" << username;
        processArgs << "--version" << version;
        processArgs << "--gameDir" << absoluteGameDir;
        processArgs << "--assetsDir" << QDir::current().absoluteFilePath("Assets/" + instanceName);
        processArgs << "--assetIndex" << assetIndex;
        processArgs << "--uuid" << uuid;
        processArgs << "--accessToken" << "0";
        processArgs << "--userType" << "mojang";

        // Append User Extra Arguments from args.txt
        processArgs << launchArgs.split(' ', Qt::SkipEmptyParts);

        mcProcess->setWorkingDirectory(instDir);

        QString fullCmd = javaPath + " " + processArgs.join(" ");
        gameConsole->appendPlainText("--- Launching Minecraft ---");
        gameConsole->appendPlainText("Command: " + fullCmd + "\n");
        LogLauncherEvent("Starting instance: " + instanceName);

        mcProcess->start(javaPath, processArgs);

        playBtn->setText("STOP");
    });

    connect(mcProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        gameConsole->appendPlainText(mcProcess->readAllStandardOutput());
    });
    connect(mcProcess, &QProcess::readyReadStandardError, this, [this]() {
        gameConsole->appendPlainText(mcProcess->readAllStandardError());
    });

    connect(mcProcess, &QProcess::finished, this, [this]() {
        playBtn->setText("PLAY");
        LogLauncherEvent("Instance process finished.");
    });

    // Switch User Section
    QVBoxLayout *userLayout = new QVBoxLayout();
    welcomeLabel = new QLabel("Welcome, Player", this);
    QPushButton *switchUser = new QPushButton("Switch User", this);
    QPushButton *settingsBtn = new QPushButton("Settings", this);

    connect(switchUser, &QPushButton::clicked, this, [this]() {
        ShowProfileWindow(this);
        updateUserLabel();
    });

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        ShowSettingsWindow(this);
    });

    userLayout->addWidget(welcomeLabel); // Keep welcomeLabel in userLayout
    userLayout->addWidget(switchUser);
    userLayout->addWidget(settingsBtn);

    bottomLayout->addLayout(profileSectionLayout);
    bottomLayout->addStretch();
    bottomLayout->addWidget(playBtn);
    bottomLayout->addStretch();
    bottomLayout->addLayout(userLayout);

    mainLayout->addLayout(bottomLayout);

    updateUserLabel();
}

MinecraftLauncher::~MinecraftLauncher() {}

void MinecraftLauncher::closeEvent(QCloseEvent *event) {
    // Ensure Minecraft is closed if the launcher is exited
    if (mcProcess && mcProcess->state() != QProcess::NotRunning) {
        mcProcess->terminate();
        if (!mcProcess->waitForFinished(2000)) mcProcess->kill();
    }
    event->accept();
}

void MinecraftLauncher::updateUserLabel() {
    QFile file("LauncherSession.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject root = doc.object();
            QString user = root.contains("profile") ? root["profile"].toObject()["name"].toString() : 
                           root.contains("username") ? root["username"].toString() : 
                           root["Username"].toString(); // Fallback for old format
            if (!user.isEmpty()) {
                welcomeLabel->setText("Welcome, " + user);
            }
        }
        file.close();
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    RJMLWebView::initialize();

    app.setApplicationName(GetAppName());

    // Prevent the application from exiting when the splash dialog closes
    app.setQuitOnLastWindowClosed(false);

    // Run Splash Logic ONLY ONCE. 
    // If it returns false, the app exits (e.g., an update was triggered).
    if (!SplashScreenLogic::run()) {
        return 0;
    }

    // Initialize the main window ONLY after the splash screen has completed.
    MinecraftLauncher w;
    
    // Transition to Main Window: Show, normalize, raise, and activate it.
    w.show();
    w.showNormal(); // Ensure it isn't starting minimized

    // On some Linux window managers, we need to defer activation slightly 
    // to prevent the OS from losing the window handle during the splash transition.
    QTimer::singleShot(100, &w, [&w]() {
        w.raise();
        w.activateWindow();
    });

    // Use a safety delay before re-enabling automatic shutdown. This prevents
    // a "sudden death" race condition where the app might quit if the OS
    // hasn't fully acknowledged the main window's presence yet.
    QTimer::singleShot(500, [&w]() {
        // Re-enable native quit behavior now that the main window is definitely stable.
        QApplication::setQuitOnLastWindowClosed(true);

        QListWidget* list = w.findChild<QListWidget*>("updateList");
        QTextBrowser* details = w.findChild<QTextBrowser*>("updateDetails");
        QPushButton* installBtn = w.findChild<QPushButton*>("installUpdateBtn");
        QTextBrowser* newsBrowser = w.findChild<QTextBrowser*>("newsBrowser");
        QListWidget* fileList = w.findChild<QListWidget*>("fileList");
        QLabel* currentVerLabel = w.findChild<QLabel*>("currentVerLabel");

        if (currentVerLabel) {
            currentVerLabel->setText("Current Installed Build: " + QString::number(GetBuildNumber()));
        }

        if (list && details && installBtn && fileList) {
            QObject::connect(list, &QListWidget::itemSelectionChanged, [list, details, installBtn, fileList]() {
                QListWidgetItem *item = list->currentItem();
                if (item) {
                    details->setMarkdown(item->data(Qt::UserRole).toString());
                    
                    fileList->clear();
                    QVariantList assets = item->data(Qt::UserRole + 1).toList();
                    for (const QVariant &v : assets) {
                        QVariantMap map = v.toMap();
                        QListWidgetItem *fItem = new QListWidgetItem(map["name"].toString(), fileList);
                        fItem->setCheckState(Qt::Unchecked);
                        fItem->setData(Qt::UserRole, map["url"].toString());
                    }
                    installBtn->setEnabled(true);
                } else {
                    installBtn->setEnabled(false);
                }
            });

            QObject::connect(installBtn, &QPushButton::clicked, [&w, list, fileList]() {
                QListWidgetItem *selectedVersionItem = list->currentItem();
                if (!selectedVersionItem) return;

                QString versionTag = selectedVersionItem->text(); // e.g., "v1.0.0 (Stable)"
                QString buildNumber = QString::number(selectedVersionItem->data(Qt::UserRole + 2).toInt());

                QList<QPair<QString, QString>> filesToDownload; // url, destinationPath
                QString downloadDir = "LauncherUpdater/LauncherSource/"; // Where downloaded zips go

                for (int i = 0; i < fileList->count(); ++i) {
                    QListWidgetItem *fileItem = fileList->item(i);
                    if (fileItem->checkState() == Qt::Checked) {
                        QString fileUrl = fileItem->data(Qt::UserRole).toString();
                        QString fileName = fileItem->text();
                        QString destinationPath = downloadDir + fileName;
                        filesToDownload.append({fileUrl, destinationPath});
                    }
                }

                if (filesToDownload.isEmpty()) {
                    QMessageBox::information(&w, "No Files Selected", "Please select at least one file to download.");
                    return;
                }

                QDialog *downloadDialog = CreateDownloadProgressDialog(filesToDownload, &w);
                ConnectDownloadDialogSignals(downloadDialog, &w, [buildNumber, &w, downloadDialog](bool success, const QList<QString>& downloadedFiles) {
                    if (success && !downloadedFiles.isEmpty()) {
                        // Use the unified handover logic from updater.cpp instead of manual extraction.
                        QString mainZip;
                        for (const QString& path : downloadedFiles) {
                            QFileInfo fi(path);
#ifdef Q_OS_WIN
                            // On Windows, identify the installer executable
                            if (fi.suffix().toLower() == "exe") {
#else
                            // On Linux/Mac, identify the update zip
                            if (fi.suffix().toLower() == "zip") {
#endif
                                mainZip = QFileInfo(path).fileName();
                                break;
                            }
                        }

                        if (!mainZip.isEmpty()) {
                            TriggerLocalUpdate(mainZip);
                        } else {
                            QMessageBox::information(&w, "Download Complete", "Package downloaded. You can apply the update from the 'Downloaded Packages' tab.");
                        }
                    } else if (!success) {
                        QMessageBox::critical(&w, "Download Failed", "Some files failed to download. Please check logs.");
                    }
                    downloadDialog->deleteLater();
                }, [&w, downloadDialog](const QList<QString>& partiallyDownloadedFiles) {
                    QMessageBox::information(&w, "Download Cancelled", "Download was cancelled. Partially downloaded files remain in 'Downloaded Packages' tab.");
                    downloadDialog->deleteLater();
                });

                downloadDialog->exec(); // Show as modal dialog
            });
        }

        UpdateInfo uInfo = CheckForUpdates();
        QString newsHtml;

        for (const auto &rel : uInfo.allReleases) {
            // Populate Tab 1: News (Aggregate descriptions)
            newsHtml += "# " + rel.tagName + (rel.isPreRelease ? " (Pre-release)" : " (Stable)") + "\n";
            newsHtml += rel.description + "\n\n---\n\n";

            // Populate Tab 2: Update Selection
            QString label = QString("%1 (%2)").arg(rel.tagName, rel.isPreRelease ? "Pre-release" : "Stable");
            QListWidgetItem *item = new QListWidgetItem(label, list);
            item->setData(Qt::UserRole, rel.description);
            
            QVariantList assetsData;
            for (const auto &asset : rel.assets) {
                QVariantMap map;
                map["name"] = asset.name;
                map["url"] = asset.url;
                assetsData.append(map);
            }
            item->setData(Qt::UserRole + 1, assetsData);
            item->setData(Qt::UserRole + 2, rel.buildNumber);
        }

        if (newsBrowser) {
            if (newsHtml.isEmpty()) newsBrowser->setText("Launcher is up to date.");
            else newsBrowser->setMarkdown(newsHtml);
        }
    });
    return app.exec();
}
