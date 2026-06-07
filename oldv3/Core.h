/**
 * Core.h - RJ Launcher Header
 */

#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProcess>
#include <QCheckBox>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QButtonGroup>

/**
 * MinecraftLauncher - Main Window for the RJ Launcher.
 * Provides a modern layout using the Qt6 Framework.
 */
class MinecraftLauncher : public QMainWindow {
    Q_OBJECT

public:
    explicit MinecraftLauncher(QWidget *parent = nullptr);
    ~MinecraftLauncher() override;

    void updateUserLabel();
    static QString getRJLDataPath();
    void launchMinecraft();
    
    // Banner status and rotation controls
    bool m_hasUpdateStatus = false;
    int m_bannerIndex = -1;
    void rotateBanner();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void loadBannerConfig();
    void refreshInstanceTab();
    void deleteGroup(const QString &gName);

    QLabel *welcomeLabel = nullptr;
    QTabWidget *tabs = nullptr;
    QPlainTextEdit *gameConsole = nullptr;
    QComboBox *profileComboBox = nullptr;
    QPushButton *playBtn = nullptr;
    QProcess *mcProcess = nullptr;

    QVBoxLayout *m_instScrollLayout = nullptr;
    QWidget *m_instScrollContent = nullptr;
    QButtonGroup *m_instanceButtonGroup = nullptr;
    QFileSystemWatcher *m_instanceWatcher = nullptr;
    QCheckBox *m_forceThemeCheck = nullptr;

    // Banner UI components and animation configuration
    QWidget *m_bannerContainer = nullptr;
    QWidget *m_bannerContent = nullptr;
    QPushButton *m_currentBanner = nullptr;
    QPushButton *m_nextBanner = nullptr;
    QTimer *m_bannerTimer = nullptr;
    QStringList m_normalTexts;
    QStringList m_updateTexts;
    int m_timeswitch = 5000;
    bool m_doSlide = true;
    bool m_doFade = true;

public slots:
    void refreshInstanceList(); // New slot to refresh instance list
};

// Global function to refresh main window UI
void RefreshMainWindowUI();

void LogLauncherEvent(const QString &message);
