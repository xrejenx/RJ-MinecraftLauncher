#include "theme.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QDialog>
#include <QSysInfo>
#include <QApplication>
#include <QDebug>
#include <QStyleHints>
#include <QStyleFactory>
#include <QStyle>
#include <QProxyStyle>
#include <QSettings>
#include "Core.h" // Corrected to reference the main Core.h in the root directory
#include "desktoptheme.h"

class HoverStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    // This proxy style can be expanded to handle state-based color transitions
    // for a "slow show" effect on native widgets.
};

void ShowCreateThemeDialog(QWidget *parent, const std::function<void()> &onCreated) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Create Custom Theme");
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    QLineEdit *colorEdit = new QLineEdit("#ffffff", &dlg);
    QLineEdit *pathEdit = new QLineEdit(&dlg);
    QPushButton *browseBtn = new QPushButton("Browse...", &dlg);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseBtn);

    layout->addRow("Theme Name:", nameEdit);
    layout->addRow("Background Color:", colorEdit);
    layout->addRow("Background Image:", pathLayout);

    QObject::connect(browseBtn, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(parent, "Select Image", "", "Images (*.png *.jpg *.jpeg)");
        if (!path.isEmpty()) pathEdit->setText(path);
    });

    QPushButton *okBtn = new QPushButton("Create", &dlg);
    layout->addWidget(okBtn);

    QObject::connect(okBtn, &QPushButton::clicked, [&]() {
        if (nameEdit->text().isEmpty()) return;
        CustomThemeInfo info{nameEdit->text(), colorEdit->text(), pathEdit->text(), true};
        ThemeLoader::createCustomTheme(info);
        if (onCreated) onCreated();
        dlg.accept();
    });

    dlg.exec();
}

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
    QStringList defaults = {"LLight", "DDark", "FLight", "FDark", "LBlue", "DBlue", "FBlue", "FDBlue"}; 
    for (const QString &name : defaults) {
        QString path = dataRoot + "LTheme/themes/" + name;
        QDir().mkpath(path + "/ThemeImage");
        
        bool isDark = name.contains("Dark") || name.contains("DBlue");
        bool isFusion = name.startsWith("F");
        bool isBlue = name.contains("Blue");

        // Create the JSON color configuration
        QFile jsonFile(path + "/theme" + name + ".json");
        if (!jsonFile.exists() && jsonFile.open(QIODevice::WriteOnly)) {
            QJsonObject theme;
            theme["theme_color_time"] = isDark ? "2" : "l";
            theme["theme_fusion_style_support"] = isFusion ? "1" : "2"; 
            theme["force_fusion_win10"] = "1";
            theme["bta_css"] = "2"; // Default CSS buttons OFF
            theme["bta_color_css"] = "2";
            theme["bta_native"] = "1";
            theme["bta_conf_allow"] = "1";
            theme["background"] = "1";
            theme["viewp_background"] = "1";
            theme["force_background_image"] = "2";
            theme["custom_banner_css"] = "2";
            theme["custom_banner_text"] = "2";
            
            if (!isDark) { // Light themes (LLight, FLight, LBlue, FBlue)
                theme["background_solid"] = "#FFFFFF";
                theme["text"] = "#000000";
                theme["accent"] = isBlue ? "#0078D7" : "#F0F0F0"; 
                theme["border"] = "#CCCCCC";
            } else { // Dark themes (DDark, FDark, DBlue, FDBlue)
                theme["background_solid"] = "#1E1E1E";
                theme["text"] = "#D4D4D4";
                theme["accent"] = isBlue ? "#0078D7" : "#2D2D2D"; 
                theme["border"] = "#454545";
            }
            
            theme["bta_css_color"] = theme["accent"];
            theme["bta_css_curve"] = "0"; // No default curve
            theme["viewp_background_color"] = theme["background_solid"];
            theme["background_image"] = "";
            theme["banner_css_theme"] = isDark ? "dark" : "light";
            theme["banner_css_color"] = theme["accent"];
            theme["banner_css_border_color"] = theme["border"];
            theme["banner_css_light_inverted"] = "1";
            theme["banner_css_force"] = "auto";
            theme["banner_text_force"] = "custom";

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
    QSettings settingsIni(dataRoot + "launcher.ini", QSettings::IniFormat);
    bool forceFusion = settingsIni.value("theme/forceFusion", false).toBool();

    QString jsonPath = dataRoot + QString("LTheme/themes/%1/theme%1.json").arg(selected);
    QJsonObject config;
    if (QFile::exists(jsonPath)) {
        QFile file(jsonPath);
        if (file.open(QIODevice::ReadOnly)) {
            config = QJsonDocument::fromJson(file.readAll()).object();
            file.close();
        }
    }

    // Determine color mode from JSON
    bool isDark = (config["theme_color_time"].toString() == "2");

    // Extract specific colors from JSON with fallbacks
    QString bgSolid = config["background_solid"].toString();
    if (bgSolid.isEmpty()) bgSolid = isDark ? "#1E1E1E" : "#FFFFFF"; // Fallback if not in JSON
    QString txtCol = config["text"].toString();
    if (txtCol.isEmpty()) txtCol = isDark ? "#D4D4D4" : "#000000"; // Fallback if not in JSON
    QString accCol = config["accent"].toString();
    if (accCol.isEmpty()) accCol = isDark ? "#2D2D2D" : "#F0F0F0"; // Fallback if not in JSON
    QString brdCol = config["border"].toString();
    if (brdCol.isEmpty()) brdCol = isDark ? "#454545" : "#CCCCCC"; // Fallback if not in JSON

    QString bannerStyle; // Initialize bannerStyle
    if (config["custom_banner_css"].toString() == "1") {
        QString bCol = config["banner_css_color"].toString();
        QString bBrd = config["banner_css_border_color"].toString();
        QString bTxt = isDark ? "#ffffff" : "#000000";
        bannerStyle = QString("QPushButton#warningBanner { background-color: %1; color: %2; border: 1px solid %3; font-weight: bold; padding: 0 20px; border-radius: %4px; outline: none; }")
                      .arg(bCol, bTxt, bBrd, config["bta_css_curve"].toString());
    } else {
        if (isDark) {
            bannerStyle = "QPushButton#warningBanner { background-color: #000000; color: #ffffff; border: 1px solid #ffffff; font-weight: bold; padding: 0 20px; border-radius: 0px; outline: none; } QPushButton#warningBanner:hover { background-color: #222222; }";
        } else {
            bannerStyle = "QPushButton#warningBanner { background-color: #f0f0f0; color: #000000; border: 1px solid #000000; font-weight: bold; padding: 0 20px; border-radius: 0px; outline: none; } QPushButton#warningBanner:hover { background-color: #e0e0e0; }";
        }
    }

    bool isWin10 = false;
#ifdef Q_OS_WIN
    if (QSysInfo::kernelVersion().split('.').value(2).toInt() < 22000) {
        isWin10 = true;
    }
#endif

    // Clear existing stylesheet to prevent conflicts with palette-based styling
    qApp->setStyleSheet("");
    QString extraStyle = "";

    // Determine style engine (Fusion vs Native)
    bool useFusion = (config["theme_fusion_style_support"].toString() == "1") || forceFusion;

    // Apply Windows 10 specific Force logic if required by JSON
#ifdef Q_OS_WIN
    if (isDark && config["force_fusion_win10"].toString() == "1" && isWin10) useFusion = true;
#endif

    // Set Style FIRST so standardPalette() is correct for the engine
    if (useFusion) QApplication::setStyle("Fusion");
    else {
#ifdef Q_OS_WIN
        QApplication::setStyle("windowsvista");
#else
        // Fallback for non-Windows native
        QApplication::setStyle(QStyleFactory::create("Fusion")); // Or another suitable default
#endif
    }

    // Palette Management
    QPalette p = qApp->style()->standardPalette(); // Get the palette for the *currently set style*
    p.setColor(QPalette::Window, QColor(bgSolid));
    p.setColor(QPalette::WindowText, QColor(txtCol));
    p.setColor(QPalette::Base, QColor(bgSolid));
    p.setColor(QPalette::Text, QColor(txtCol));
    p.setColor(QPalette::Button, QColor(accCol));
    p.setColor(QPalette::ButtonText, QColor(txtCol));
    p.setColor(QPalette::Highlight, QColor(accCol));
    p.setColor(QPalette::HighlightedText, isDark ? Qt::white : Qt::white);
    p.setColor(QPalette::HighlightedText, Qt::white);

    if (isDark) {
        p.setColor(QPalette::AlternateBase, QColor(bgSolid).darker(110));
        p.setColor(QPalette::ToolTipBase, Qt::white);
        p.setColor(QPalette::ToolTipText, Qt::white);
    } else {
        // For light themes, ensure alternate base is visible
        p.setColor(QPalette::AlternateBase, QColor(bgSolid).darker(105));
    }
    qApp->setPalette(p);

    // Apply CSS for buttons if bta_css is enabled
    if (config["bta_css"].toString() == "1") {
        QString btnCol = config["bta_color_css"].toString() == "1" ? config["bta_css_color"].toString() : accCol;
        QString curve = config["bta_css_curve"].toString();
        extraStyle += QString("QPushButton { background-color: %1; border-radius: %2px; border: 1px solid %3; padding: 5px; }")
                      .arg(btnCol, curve, brdCol);
        extraStyle += QString("QPushButton:hover { background-color: %1; }").arg(QColor(btnCol).lighter(110).name());
    }

    // Generic Control Styling for Dark Fusion
    if (isDark && useFusion) {
        extraStyle += QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; }"
            "QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; }"
            "QAbstractItemView::item:hover { background-color: %4; }" // Ensure hover uses accent color
            "QLineEdit, QSpinBox, QPlainTextEdit { background-color: %1; color: %2; border: 1px solid %3; }"
            "QProgressBar { border: 1px solid %3; text-align: center; background: black; }"
            "QProgressBar::chunk { background: %4; }"
            "QListWidget::item:selected, QListView::item:selected, QTreeView::item:selected { background-color: %4; color: white; }"
        ).arg(bgSolid, txtCol, brdCol, accCol);
    }

    // Background Logic (Mode 1=Solid, 2=Solid, 3=None, 4=External)
    QString bgMode = config["background"].toString();
    if (bgMode == "1" || bgMode == "2") {
        QString img = config["background_image"].toString();
        if (config["force_background_image"].toString() == "1" && !img.isEmpty()) {
            QString imgPath = dataRoot + "LTheme/themes/" + selected + "/" + img;
            extraStyle += QString("QMainWindow { border-image: url(%1) 0 0 0 0 stretch stretch; }").arg(imgPath);
        } else {
            extraStyle += QString("QMainWindow { background-color: %1; }").arg(bgSolid);
        }
    } else if (bgMode.startsWith("4")) {
        QString otherTheme = bgMode.section(',', 1).trimmed();
        QString otherJson = dataRoot + "LTheme/themes/" + otherTheme + "/theme" + otherTheme + ".json";
        QFile f(otherJson);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
            QString oImg = o["background_image"].toString();
            if (!oImg.isEmpty()) {
                extraStyle += QString("QMainWindow { border-image: url(%1) 0 0 0 0 stretch stretch; }").arg(dataRoot + "LTheme/themes/" + otherTheme + "/" + oImg);
            }
        }
    }

    qApp->setStyleSheet(bannerStyle + extraStyle);
}

QStringList ThemeLoader::getAvailableThemes() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QDir themesDir(dataRoot + "LTheme/themes");
    QStringList themes;
    for (const QString &dirName : themesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir subdir(themesDir.absoluteFilePath(dirName));
        if (!subdir.entryList({"*.cfg"}).isEmpty()) {
            themes << dirName;
        }
    }
    return themes;
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
    theme["theme_color_time"] = info.autoDetect ? "l" : "2";
    theme["theme_fusion_style_support"] = "2";
    theme["force_fusion_win10"] = "1";
    theme["bta_css"] = "1";
    theme["bta_color_css"] = "1";
    theme["bta_native"] = "1";
    theme["bta_conf_allow"] = "1";
    theme["background"] = "1";
    theme["viewp_background"] = "1";
    theme["force_background_image"] = info.imagePath.isEmpty() ? "2" : "1";
    theme["custom_banner_css"] = "2";
    theme["custom_banner_text"] = "2";

    theme["background_solid"] = info.color;
    theme["text"] = info.autoDetect ? "#000000" : "#FFFFFF";
    theme["accent"] = info.color;
    theme["border"] = info.autoDetect ? "#CCCCCC" : "#454545";
    theme["background_image"] = info.imagePath;
    theme["bta_css_color"] = info.color;
    theme["bta_css_curve"] = "4";

    QFile jsonFile(themePath + "/theme" + info.name + ".json");
    if (jsonFile.open(QIODevice::WriteOnly)) {
        jsonFile.write(QJsonDocument(theme).toJson());
        jsonFile.close();
    }

    // Create the .cfg metadata file as requested for folder-based theme creation
    QFile cfgFile(themePath + "/theme" + info.name + ".cfg");
    if (cfgFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&cfgFile);
        out << "theme_name=" << info.name << "\n";
        out << "author=User_Custom\n";
        cfgFile.close();
    }
}

QString ThemeLoader::getSelectedTheme() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QFile file(dataRoot + "LTheme/theme.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        return obj["selectedTheme"].toString("LLight");
    }
    return "LLight";
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
