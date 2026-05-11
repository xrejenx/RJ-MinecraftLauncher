#include "theme.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFormLayout> // Added for QFormLayout
#include <QLineEdit>   // Added for QLineEdit
#include <QPushButton> // Added for QPushButton
#include <QHBoxLayout> // Added for QHBoxLayout
#include <QComboBox>   // Added for QComboBox
#include <QFileDialog> // Added for QFileDialog
#include <QFileInfo>   // Added for QFileInfo
#include <QTextStream>
#include <QDebug>
#include <QApplication>
#include "Core.h" // Corrected to reference the main Core.h in the root directory
#include "LTheme/desktoptheme.h"

void ThemeLoader::initialize() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    // Ensure the folder structure exists
    QDir().mkpath(dataRoot + "LTheme/themes");
    
    QFile configFile(dataRoot + "LTheme/theme.json");
    if (!configFile.exists()) {
        if (configFile.open(QIODevice::WriteOnly)) {
            QJsonObject obj;
            obj["selectedTheme"] = getAutoThemeName();
            configFile.write(QJsonDocument(obj).toJson());
            configFile.close();
        }
    }
    
    createDefaultThemes();
    applyTheme();
}

void ThemeLoader::createDefaultThemes() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QStringList defaults = {"Llight", "Ddark"};
    for (const QString &name : defaults) {
        QString path = dataRoot + "LTheme/themes/" + name;
        QDir().mkpath(path + "/ThemeImage");
        
        // Create the JSON color configuration
        QFile jsonFile(path + "/theme" + name + ".json");
        if (!jsonFile.exists() && jsonFile.open(QIODevice::WriteOnly)) {
            QJsonObject theme;
            if (name == "Llight") {
                theme["background"] = "#FFFFFF";
                theme["text"] = "#000000";
                theme["accent"] = "#F0F0F0";
                theme["border"] = "#CCCCCC";
                theme["backgroundImage"] = "";
            } else {
                theme["background"] = "#1E1E1E";
                theme["text"] = "#D4D4D4";
                theme["accent"] = "#2D2D2D";
                theme["border"] = "#454545";
                theme["backgroundImage"] = "";
            }
            jsonFile.write(QJsonDocument(theme).toJson());
            jsonFile.close();
        }
        
        // Create the .cfg metadata file
        QFile cfgFile(path + "/theme" + name + ".cfg");
        if (!cfgFile.exists() && cfgFile.open(QIODevice::WriteOnly)) {
            QTextStream out(&cfgFile);
            out << "theme_name=" << name << "\n";
            out << "author=RJML_Default\n";
            cfgFile.close();
        }
    }
}

void ThemeLoader::applyTheme() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QString selected = getSelectedTheme();

    // Persistent Banner Style: Black frame with white outline
    QString bannerStyle = "QLabel#warningBanner { background-color: #000000; color: #ffffff; border: 1px solid #ffffff; font-weight: bold; }";

    if (selected == "Ddark" || selected == "Llight") {
        qApp->setStyleSheet(bannerStyle);
        
        if (selected == "Ddark") {
            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, QColor(15, 15, 15));
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, Qt::black);
            darkPalette.setColor(QPalette::AlternateBase, QColor(15, 15, 15));
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Button, Qt::black); 
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            darkPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(darkPalette);
        } else {
            QPalette lightPalette;
            lightPalette.setColor(QPalette::Window, Qt::white);
            lightPalette.setColor(QPalette::WindowText, Qt::black);
            lightPalette.setColor(QPalette::Base, Qt::white);
            lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
            lightPalette.setColor(QPalette::Text, Qt::black);
            lightPalette.setColor(QPalette::Button, Qt::white); 
            lightPalette.setColor(QPalette::ButtonText, Qt::black);
            lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215)); 
            lightPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(lightPalette);
        }
        return;
    }

    // Custom CSS themes (legacy fallback)
    // ... [Logic kept same as main Themeloader.cpp] ...
}

QStringList ThemeLoader::getAvailableThemes() {
    return QDir(MinecraftLauncher::getRJLDataPath() + "LTheme/themes").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QString ThemeLoader::getThemeImagePath(const QString &themeName) {
    QFile file(MinecraftLauncher::getRJLDataPath() + QString("LTheme/themes/%1/theme%1.json").arg(themeName));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        return obj["backgroundImage"].toString();
    }
    return "";
}

void ThemeLoader::createCustomTheme(const CustomThemeInfo &info) {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QString themePath = dataRoot + "LTheme/themes/" + info.name;
    QDir().mkpath(themePath + "/ThemeImage");

    QJsonObject theme;
    theme["background"] = info.color;
    theme["text"] = info.autoDetect ? "#000000" : "#FFFFFF";
    theme["backgroundImage"] = info.imagePath;

    QFile jsonFile(themePath + "/theme" + info.name + ".json");
    if (jsonFile.open(QIODevice::WriteOnly)) {
        jsonFile.write(QJsonDocument(theme).toJson());
        jsonFile.close();
    }
}

QString ThemeLoader::getSelectedTheme() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QFile file(dataRoot + "LTheme/theme.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        return obj["selectedTheme"].toString("Llight");
    }
    return "Llight";
}

void ThemeLoader::setSelectedTheme(const QString &themeName) {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QFile file(dataRoot + "LTheme/theme.json");
    if (file.open(QIODevice::ReadWrite)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        obj["selectedTheme"] = themeName;
        file.seek(0);
        file.write(QJsonDocument(obj).toJson());
        file.resize(file.pos());
        file.close();
    }
}