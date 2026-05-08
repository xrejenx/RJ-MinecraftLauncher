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

void ThemeLoader::initialize() {
    // Ensure the folder structure exists
    QDir().mkpath("LTheme/themes");
    
    QFile configFile("LTheme/theme.json");
    if (!configFile.exists()) {
        if (configFile.open(QIODevice::WriteOnly)) {
            QJsonObject obj;
            obj["selectedTheme"] = "LLight";
            configFile.write(QJsonDocument(obj).toJson());
            configFile.close();
        }
    }
    
    createDefaultThemes();
    applyTheme();
}

void ThemeLoader::createDefaultThemes() {
    QStringList defaults = {"LLight", "DDark"};
    for (const QString &name : defaults) {
        QString path = "LTheme/themes/" + name;
        QDir().mkpath(path + "/ThemeImage");
        
        // Create the JSON color configuration
        QFile jsonFile(path + "/theme" + name + ".json");
        if (!jsonFile.exists() && jsonFile.open(QIODevice::WriteOnly)) {
            QJsonObject theme;
            if (name == "LLight") {
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
    QString selected = getSelectedTheme();
    QString jsonPath = QString("LTheme/themes/%1/theme%1.json").arg(selected);
    
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonObject colors = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    
    QString bg = colors["background"].toString();
    QString fg = colors["text"].toString();
    QString acc = colors["accent"].toString();
    QString brd = colors["border"].toString();
    QString img = colors["backgroundImage"].toString();

    QString windowStyle = img.isEmpty() ? 
        QString("QMainWindow { background-color: %1; }").arg(bg) :
        QString("QMainWindow { border-image: url(%1) 0 0 0 0 stretch stretch; }").arg(img);
    
    qApp->setStyleSheet(windowStyle);
}

QStringList ThemeLoader::getAvailableThemes() {
    return QDir("LTheme/themes").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QString ThemeLoader::getThemeImagePath(const QString &themeName) {
    QFile file(QString("LTheme/themes/%1/theme%1.json").arg(themeName));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        return obj["backgroundImage"].toString();
    }
    return "";
}

void ThemeLoader::createCustomTheme(const CustomThemeInfo &info) {
    QString themePath = "LTheme/themes/" + info.name;
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
    QFile file("LTheme/theme.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        return obj["selectedTheme"].toString("LLight");
    }
    return "LLight";
}

void ThemeLoader::setSelectedTheme(const QString &themeName) {
    QFile file("LTheme/theme.json");
    if (file.open(QIODevice::ReadWrite)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        obj["selectedTheme"] = themeName;
        file.seek(0);
        file.write(QJsonDocument(obj).toJson());
        file.resize(file.pos());
        file.close();
    }
}

void ShowCreateThemeDialog(QWidget *parent, const std::function<void()> &onCreated) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Create New Theme");
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    QLineEdit *imgEdit = new QLineEdit(&dlg);
    QPushButton *browseBtn = new QPushButton("Browse", &dlg);
    
    QHBoxLayout *imgLayout = new QHBoxLayout();
    imgLayout->addWidget(imgEdit);
    imgLayout->addWidget(browseBtn);

    QComboBox *colorMode = new QComboBox(&dlg);
    colorMode->addItems({"Auto-Detect from Picture", "Solid Color"});

    QPushButton *doneBtn = new QPushButton("Done", &dlg);

    layout->addRow("Theme Name:", nameEdit);
    layout->addRow("Background Image:", imgLayout);
    layout->addRow("Color Set:", colorMode);
    layout->addWidget(doneBtn);

    QObject::connect(browseBtn, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(&dlg, "Select Image", "", "Images (*.png *.jpg)");
        if (!path.isEmpty()) imgEdit->setText(path);
    });

    QObject::connect(doneBtn, &QPushButton::clicked, [&]() {
        if (nameEdit->text().isEmpty()) return;
        
        QString themeName = nameEdit->text();
        QString themePath = "LTheme/themes/" + themeName;
        QDir().mkpath(themePath + "/ThemeImage");

        // Copy image if selected
        QString finalImgPath = "";
        if (!imgEdit->text().isEmpty()) {
            QString ext = QFileInfo(imgEdit->text()).suffix();
            finalImgPath = themePath + "/ThemeImage/background." + ext;
            QFile::copy(imgEdit->text(), finalImgPath);
        }

        QJsonObject theme;
        theme["background"] = "#FFFFFF";
        theme["text"] = (colorMode->currentIndex() == 0) ? "#000000" : "#FFFFFF";
        theme["accent"] = "#A0A0A0";
        theme["border"] = "#505050";
        theme["backgroundImage"] = finalImgPath;

        QFile jsonFile(themePath + "/theme" + themeName + ".json");
        if (jsonFile.open(QIODevice::WriteOnly)) {
            jsonFile.write(QJsonDocument(theme).toJson());
            jsonFile.close();
        }

        QFile cfgFile(themePath + "/theme" + themeName + ".cfg");
        if (cfgFile.open(QIODevice::WriteOnly)) {
            QTextStream out(&cfgFile);
            out << "theme_name=" << themeName << "\n";
            cfgFile.close();
        }

        onCreated();
        dlg.accept();
    });

    dlg.exec();
}