#include "Core.h"
#include "theme.h"
#include "icon.h"
#include <QVBoxLayout>
#include <QDir>
#include <QSettings>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

// External UI component creators
QWidget* CreateModernUpdateTab(QWidget *parent);
void PopulateInstanceList(QComboBox *comboBox);
void ShowJavaProfileWindow(QWidget *parent);
void ShowProfileWindow(QWidget *parent);
void ShowSettingsWindow(QWidget *parent);
void ShowCreateThemeDialog(QWidget *parent, const std::function<void()> &onCreated);
QString GetLauncherTitle();
void SyncExtractionLibrary();
void EnsureConsoleVisibility();
QString GetAppName();

MinecraftLauncher::MinecraftLauncher(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(GetLauncherTitle());
    setFixedSize(850, 600);
    
    // Inline initialization logic
    QString dataRoot = getRJLDataPath();
    QDir().mkpath(dataRoot + "javas");
    QDir().mkpath(dataRoot + "Lib");
    QDir().mkpath(dataRoot + "Instances");
    QDir().mkpath(dataRoot + "userdata");
    QDir().mkpath(dataRoot + "LauncherUpdater/LauncherSource/old");
    QDir().mkpath(dataRoot + "Assets");
    QDir().mkpath(dataRoot + "icons");
    
    ThemeLoader::initialize();
    QSettings settings(dataRoot + "launcher.ini", QSettings::IniFormat);
    if (!settings.value("theme/disableAutoColor", false).toBool() || !QFile::exists(dataRoot + "LTheme/theme.json")) {
        ThemeLoader::setSelectedTheme(getAutoThemeName());
        ThemeLoader::applyTheme();
    }
    SyncExtractionLibrary();
    EnsureConsoleVisibility();

    ApplyLauncherIcon(this);
    setupUI();
    updateUserLabel();
}

void MinecraftLauncher::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QLabel *warningBanner = new QLabel("Welcome to " + GetAppName(), this);
    warningBanner->setStyleSheet("background-color: rgb(77, 77, 77); color: rgb(194, 194, 194); border: 1px solid white; padding: 5px;");
    warningBanner->setAlignment(Qt::AlignCenter);
    warningBanner->setFixedHeight(30);
    mainLayout->addWidget(warningBanner);

    tabs = new QTabWidget(this);
    tabs->addTab(new QLabel("News content goes here...", this), "Update Notes");
    gameConsole = new QPlainTextEdit(this);
    gameConsole->setReadOnly(true);
    gameConsole->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;");
    tabs->addTab(gameConsole, "Game Output");
    tabs->addTab(CreateModernUpdateTab(this), "Launcher Update");

    // Theme Tab construction...
    QWidget *themeTab = new QWidget(this);
    // ... (rest of UI layout logic) ...
    tabs->addTab(themeTab, "Theme");
    mainLayout->addWidget(tabs, 1);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QVBoxLayout *profileSectionLayout = new QVBoxLayout(); 
    profileComboBox = new QComboBox(this);
    PopulateInstanceList(profileComboBox);
    
    QPushButton *newProfile = new QPushButton("New Profile", this);
    profileButtonsLayout->addWidget(newProfile);

    connect(newProfile, &QPushButton::clicked, this, [this]() {
        ShowJavaProfileWindow(this);
        PopulateInstanceList(profileComboBox);
        updateUserLabel();
    });

    mcProcess = new QProcess(this);
    playBtn = new QPushButton("PLAY", this);
    playBtn->setFixedSize(200, 60);
    QFont playFont = playBtn->font();
    playFont.setBold(true); playFont.setPointSize(18);
    playBtn->setFont(playFont);

    connect(playBtn, &QPushButton::clicked, this, &MinecraftLauncher::launchMinecraft);

    bottomLayout->addLayout(profileSectionLayout);
    bottomLayout->addStretch();
    bottomLayout->addWidget(playBtn);
    bottomLayout->addStretch();
    mainLayout->addLayout(bottomLayout);
}

MinecraftLauncher::~MinecraftLauncher() {}

void MinecraftLauncher::closeEvent(QCloseEvent *event) {
    if (mcProcess && mcProcess->state() != QProcess::NotRunning) {
        mcProcess->terminate();
        if (!mcProcess->waitForFinished(2000)) mcProcess->kill();
    }
    event->accept();
}