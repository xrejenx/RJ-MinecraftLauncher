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
#include <QJsonObject>
#include <QScrollArea>
#include <QFile>
#include <QDir>
#include <QFileInfo> // Ensure QFileInfo is included
#include <QSettings>
#include <QProcess>
#include "theme.h"
#include "Core.h"

// External function interfaces
void ShowJavaProfileWindow(QWidget *parent);
void LogLauncherEvent(const QString &message);
void ShowProfileWindow(QWidget *parent);
void ShowSettingsWindow(QWidget *parent);
void EnsureConsoleVisibility();
QString GetLauncherTitle(); // From LauncherPatch/version.cpp
QString GetAppName();      // From LauncherPatch/version.cpp
QString GetLatestUpdateNote();
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0); // From instance-javaRequirement.cpp
QString GetLwglVersionForMc(const QString &mcVersion); // From lgwl-handlelib.cpp
void PopulateInstanceList(QComboBox *comboBox); // Changed from QListWidget
QString GetLwglNativesPath(const QString &mcVersion); // Forward declaration for LWJGL path

/**
 * Core.cpp - RJ Launcher (Base)
 * 
 * This file provides the entry point and UI layout for the launcher.
 */

MinecraftLauncher::MinecraftLauncher(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(GetLauncherTitle());
    setFixedSize(850, 600);
    
    // Ensure important folders exist on launch
    QDir().mkpath("javas");
    QDir().mkpath("Lib");
    QDir().mkpath("Instances");
    QDir().mkpath("userdata");
    QDir().mkpath("Assets");
    QDir().mkpath("icons");
    
    // Initialize Theme System
    ThemeLoader::initialize();

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
    
    QScrollArea *updateScroll = new QScrollArea(this);
    QLabel *updateNoteLabel = new QLabel(GetLatestUpdateNote(), updateScroll);
    updateNoteLabel->setWordWrap(true);
    updateNoteLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    updateNoteLabel->setStyleSheet("padding: 10px; background: white;");
    updateScroll->setWidget(updateNoteLabel);
    updateScroll->setWidgetResizable(true);
    tabs->addTab(updateScroll, "LauncherUpdate");

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
    
    app.setApplicationName(GetAppName());
    
    MinecraftLauncher w;
    w.show();
    
    return app.exec();
}
