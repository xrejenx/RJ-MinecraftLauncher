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
#include <QCloseEvent>

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
    void launchMinecraft(); // Logic moved to CoreLaunch.cpp

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();           // Logic moved to Core.cpp

    QLabel *welcomeLabel;
    QTabWidget *tabs;
    QPlainTextEdit *gameConsole;
    QComboBox *profileComboBox;
    QPushButton *playBtn;
    QProcess *mcProcess;
};
