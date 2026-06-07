#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QJsonObject>
#include <QJsonDocument>
#include <QComboBox>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFormLayout>
#include <QDialog>
#include <QColorDialog>
#include <QScrollArea>
#include "Core.h"

/**
 * ThemeMakerCore.cpp - Standalone Theme Customization Tool
 */

class ThemeMaker : public QWidget {
    Q_OBJECT
public:
    ThemeMaker(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("RJ Theme Maker");
        setFixedSize(880, 620);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);

        // Left Panel: Theme List
        QVBoxLayout *leftLayout = new QVBoxLayout();
        leftLayout->setSpacing(5);
        leftLayout->addWidget(new QLabel("<b>Project Themes:</b>"));
        themeList = new QListWidget();
        themeList->setAlternatingRowColors(true);
        themeList->setStyleSheet("QListWidget { border: 1px solid palette(mid); border-radius: 4px; }");
        leftLayout->addWidget(themeList);

        QGroupBox *actionGroup = new QGroupBox("Manage");
        QVBoxLayout *actLay = new QVBoxLayout(actionGroup);
        QPushButton *btnCreate = new QPushButton("CREATE NEW");
        QPushButton *btnSave = new QPushButton("SAVE CHANGES");
        btnSave->setStyleSheet("font-weight: bold; color: #0078D7;");
        actLay->addWidget(btnCreate);
        actLay->addWidget(btnSave);
        leftLayout->addWidget(actionGroup);

        mainLayout->addLayout(leftLayout, 1);

        // Right Panel: Editor
        editorTabs = new QTabWidget();
        setupEditorTabs();
        mainLayout->addWidget(editorTabs, 3);

        connect(btnCreate, &QPushButton::clicked, this, &ThemeMaker::showCreateDialog);
        connect(btnSave, &QPushButton::clicked, this, &ThemeMaker::saveTheme);
        connect(themeList, &QListWidget::currentTextChanged, this, &ThemeMaker::loadTheme);

        refreshThemeList();
    }

private slots:
    void refreshThemeList() {
        themeList->clear();
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir dir(dataRoot + "LTheme/themes");
        for (const QString &d : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!QDir(dir.absoluteFilePath(d)).entryList({"*.cfg"}).isEmpty()) {
                themeList->addItem(d);
            }
        }
    }

    void showCreateDialog() {
        QDialog dlg(this);
        dlg.setWindowTitle("Create New Theme");
        QFormLayout *lay = new QFormLayout(&dlg);

        QLineEdit *nameEdit = new QLineEdit();
        QLineEdit *authEdit = new QLineEdit();
        QCheckBox *darkCheck = new QCheckBox("Target Dark");
        QCheckBox *lightCheck = new QCheckBox("Target Light");
        QCheckBox *fusionCheck = new QCheckBox("Fusion Support");

        connect(darkCheck, &QCheckBox::toggled, [lightCheck](bool s){ if(s) lightCheck->setChecked(false); });
        connect(lightCheck, &QCheckBox::toggled, [darkCheck](bool s){ if(s) darkCheck->setChecked(false); });

        lay->addRow("Project Name:", nameEdit);
        lay->addRow("Author Name:", authEdit);
        lay->addRow(darkCheck);
        lay->addRow(lightCheck);
        lay->addRow(fusionCheck);

        QHBoxLayout *btns = new QHBoxLayout();
        QPushButton *btnCancel = new QPushButton("Cancel");
        QPushButton *btnOk = new QPushButton("Create");
        btns->addWidget(btnCancel); btns->addWidget(btnOk);
        lay->addRow(btns);

        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, [this, &dlg, nameEdit, authEdit, darkCheck, lightCheck, fusionCheck]() {
            QString name = nameEdit->text();
            if (name.isEmpty()) return;
            
            QString dataRoot = MinecraftLauncher::getRJLDataPath();
            QString path = dataRoot + "LTheme/themes/" + name;
            QDir().mkpath(path);
            QDir().mkpath(path + "/ThemeImage");

            QFile cfg(path + "/theme" + name + ".cfg");
            if (cfg.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&cfg);
                out << "theme_name=" << name << "\n";
                out << "author=" << authEdit->text() << "\n";
                cfg.close();
            }

            QJsonObject json;
            json["theme_color_time"] = darkCheck->isChecked() ? "2" : "l";
            json["theme_fusion_style_support"] = fusionCheck->isChecked() ? "1" : "2";
            json["force_fusion_win10"] = "1";
            json["bta_css"] = "2";
            json["bta_color_css"] = "2";
            json["bta_conf_allow"] = "1";
            json["background"] = "1";
            json["viewp_background"] = "1";
            json["force_background_image"] = "2";
            json["custom_banner_css"] = "2";
            json["custom_banner_text"] = "2";
            json["background_solid"] = darkCheck->isChecked() ? "#1E1E1E" : "#FFFFFF";
            json["text"] = darkCheck->isChecked() ? "#D4D4D4" : "#000000";
            json["accent"] = "#0078D7";
            json["border"] = "#454545";
            json["bta_css_curve"] = "0"; // Default creation now has 0 curve
            QFile f(path + "/theme" + name + ".json");
            if (f.open(QIODevice::WriteOnly)) {
                f.write(QJsonDocument(json).toJson());
                f.close();
            }

            refreshThemeList();
            dlg.accept();
        });

        dlg.exec();
    }

    void loadTheme(const QString &themeDir) {
        currentTheme = themeDir;
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QFile f(dataRoot + "LTheme/themes/" + themeDir + "/theme" + themeDir + ".json");
        if (f.open(QIODevice::ReadOnly)) {
            currentConfig = QJsonDocument::fromJson(f.readAll()).object();
            f.close();
            syncUiToConfig();
        }
    }

    void saveTheme() {
        if (currentTheme.isEmpty()) return;
        syncConfigToUi();
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QFile f(dataRoot + "LTheme/themes/" + currentTheme + "/theme" + currentTheme + ".json");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(currentConfig).toJson());
            f.close();
        }
    }

    void setupEditorTabs() {
        auto addColorRow = [this](QFormLayout *layout, const QString &label, QLineEdit *edit) {
            QHBoxLayout *h = new QHBoxLayout();
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(2);

            QLabel *previewBox = new QLabel();
            previewBox->setFixedSize(24, 24);
            previewBox->setFrameShape(QFrame::StyledPanel);
            
            auto updatePreview = [previewBox, edit]() {
                QString hex = edit->text().trimmed();
                if (!hex.startsWith("#")) hex = "#" + hex;
                if (QColor::isValidColorName(hex)) {
                    previewBox->setStyleSheet(QString("background-color: %1; border: 1px solid palette(mid);").arg(hex));
                }
            };
            
            connect(edit, &QLineEdit::textChanged, updatePreview);
            updatePreview();

            h->addWidget(previewBox);
            h->addWidget(edit);
            QPushButton *btn = new QPushButton("PICK");
            btn->setFixedWidth(45);
            connect(btn, &QPushButton::clicked, [edit, updatePreview]() {
                QColor c = QColorDialog::getColor(QColor(edit->text().isEmpty() ? "#FFFFFF" : edit->text()), edit->window(), "Select Color");
                if (c.isValid()) {
                    edit->setText(c.name(QColor::HexRgb).toUpper());
                    updatePreview();
                }
            });
            h->addWidget(btn);
            layout->addRow(label, h);
        };

        // Config Tab (Detailed Engine Settings)
        QWidget *confTab = new QWidget();
        QFormLayout *confLay = new QFormLayout(confTab);
        theme_color_time = new QComboBox();
        theme_color_time->addItems({"Light (l)", "Dark (2)"});
        
        theme_fusion_style_support = new QComboBox();
        theme_fusion_style_support->addItems({"On/Fusion (1)", "Off/Native (2)"});
        
        force_fusion_win10 = new QComboBox();
        force_fusion_win10->addItems({"Enable (1)", "Disable (2)"});
        
        bta_css = new QComboBox();
        bta_css->addItems({"On (1)", "Off (2)"});
        
        bta_color_css = new QComboBox();
        bta_color_css->addItems({"On (1)", "Off (2)"});
        
        bta_native = new QComboBox();
        bta_native->addItems({"Fusion (1)", "Fusion Light (2)"});
        
        bta_conf_allow = new QComboBox();
        bta_conf_allow->addItems({"Allow (1)", "Deny (2)", "Conditional (3)"});
        
        background_mode = new QComboBox();
        background_mode->addItems({"Solid (1)", "Solid (2)", "None (3)", "External (4)"});
        
        viewp_background_mode = new QComboBox();
        viewp_background_mode->addItems({"Fusion (1)", "Fusion (2)", "Native (3)", "External (4)"});
        
        force_background_image = new QComboBox();
        force_background_image->addItems({"Enable (1)", "Disable (2)"});
        
        custom_banner_css = new QComboBox();
        custom_banner_css->addItems({"Enable (1)", "Disable (2)"});
        
        custom_banner_text = new QComboBox();
        custom_banner_text->addItems({"Enable (1)", "Disable (2)"});

        confLay->addRow("Color Mode (2=Dark, l=Light):", theme_color_time);
        confLay->addRow("Fusion Style (1=On, 2=Off):", theme_fusion_style_support);
        confLay->addRow("Force Fusion Win10:", force_fusion_win10);
        confLay->addRow("Button CSS (1=On):", bta_css);
        confLay->addRow("Button Color CSS:", bta_color_css);
        confLay->addRow("Button Native Mode:", bta_native);
        confLay->addRow("Button Conf Allow:", bta_conf_allow);
        confLay->addRow("Background Mode (1-4):", background_mode);
        confLay->addRow("Viewport BG Mode:", viewp_background_mode);
        confLay->addRow("Force BG Image:", force_background_image);
        confLay->addRow("Custom Banner CSS:", custom_banner_css);
        confLay->addRow("Custom Banner Text:", custom_banner_text);
        
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidget(confTab);
        scroll->setWidgetResizable(true);
        editorTabs->addTab(scroll, "Config");

        // Button Tab
        QWidget *btnTab = new QWidget();
        QFormLayout *btnLay = new QFormLayout(btnTab);
        text_color = new QLineEdit();
        accent_color = new QLineEdit();
        border_color = new QLineEdit();
        bta_css_color = new QLineEdit();
        bta_css_curve = new QLineEdit();
        addColorRow(btnLay, "Text Color:", text_color);
        addColorRow(btnLay, "Accent/Selection Color:", accent_color);
        addColorRow(btnLay, "Border Color:", border_color);
        addColorRow(btnLay, "Button Color:", bta_css_color);
        btnLay->addRow("Button Curve (px):", bta_css_curve);
        editorTabs->addTab(btnTab, "Button");

        // Guis Tab
        QWidget *guiTab = new QWidget();
        QFormLayout *guiLay = new QFormLayout(guiTab);
        background_solid = new QLineEdit();
        viewp_background_color = new QLineEdit();
        background_image = new QLineEdit();
        addColorRow(guiLay, "Solid BG Color:", background_solid);
        addColorRow(guiLay, "Viewport BG Color:", viewp_background_color);
        guiLay->addRow("BG Image Path:", background_image);
        editorTabs->addTab(guiTab, "Guis");

        // Banner Tab
        QWidget *banTab = new QWidget();
        QFormLayout *banLay = new QFormLayout(banTab);
        banner_css_color = new QLineEdit();
        banner_css_border = new QLineEdit();
        banner_css_theme = new QLineEdit();
        banner_css_light_inverted = new QLineEdit();
        banner_css_force = new QLineEdit();
        banner_text_force = new QLineEdit();

        addColorRow(banLay, "Banner Color:", banner_css_color);
        addColorRow(banLay, "Banner Border:", banner_css_border);
        banLay->addRow("Banner Theme (dark/light/auto):", banner_css_theme);
        banLay->addRow("Banner Inverted:", banner_css_light_inverted);
        banLay->addRow("Banner Force:", banner_css_force);
        banLay->addRow("Banner Text Force:", banner_text_force);
        editorTabs->addTab(banTab, "Banner");
    }

    void syncUiToConfig() {
        // Config Tab
        theme_color_time->setCurrentIndex(currentConfig["theme_color_time"].toString() == "2" ? 1 : 0);
        theme_fusion_style_support->setCurrentIndex(currentConfig["theme_fusion_style_support"].toString() == "2" ? 1 : 0);
        force_fusion_win10->setCurrentIndex(currentConfig["force_fusion_win10"].toString() == "2" ? 1 : 0);
        bta_css->setCurrentIndex(currentConfig["bta_css"].toString() == "2" ? 1 : 0);
        bta_color_css->setCurrentIndex(currentConfig["bta_color_css"].toString() == "2" ? 1 : 0);
        bta_native->setCurrentIndex(currentConfig["bta_native"].toString() == "2" ? 1 : 0);
        bta_conf_allow->setCurrentIndex(currentConfig["bta_conf_allow"].toInt() - 1);
        background_mode->setCurrentIndex(currentConfig["background"].toInt() - 1);
        viewp_background_mode->setCurrentIndex(currentConfig["viewp_background"].toInt() - 1);
        force_background_image->setCurrentIndex(currentConfig["force_background_image"].toString() == "2" ? 1 : 0);
        custom_banner_css->setCurrentIndex(currentConfig["custom_banner_css"].toString() == "2" ? 1 : 0);
        custom_banner_text->setCurrentIndex(currentConfig["custom_banner_text"].toString() == "2" ? 1 : 0);

        // Button/UI Tab
        accent_color->setText(currentConfig["accent"].toString());
        border_color->setText(currentConfig["border"].toString());
        text_color->setText(currentConfig["text"].toString());
        bta_css_color->setText(currentConfig["bta_css_color"].toString());
        bta_css_curve->setText(currentConfig["bta_css_curve"].toString());
        background_solid->setText(currentConfig["background_solid"].toString());
        viewp_background_color->setText(currentConfig["viewp_background_color"].toString());
        background_image->setText(currentConfig["background_image"].toString());
        
        banner_css_color->setText(currentConfig["banner_css_color"].toString());
        banner_css_border->setText(currentConfig["banner_css_border_color"].toString());
        banner_css_theme->setText(currentConfig["banner_css_theme"].toString());
        banner_css_light_inverted->setText(currentConfig["banner_css_light_inverted"].toString());
        banner_css_force->setText(currentConfig["banner_css_force"].toString());
        banner_text_force->setText(currentConfig["banner_text_force"].toString());
    }

    void syncConfigToUi() {
        // Config Keys
        currentConfig["theme_color_time"] = theme_color_time->currentIndex() == 1 ? "2" : "l";
        currentConfig["theme_fusion_style_support"] = theme_fusion_style_support->currentIndex() == 1 ? "2" : "1";
        currentConfig["force_fusion_win10"] = force_fusion_win10->currentIndex() == 1 ? "2" : "1";
        currentConfig["bta_css"] = bta_css->currentIndex() == 1 ? "2" : "1";
        currentConfig["bta_color_css"] = bta_color_css->currentIndex() == 1 ? "2" : "1";
        currentConfig["bta_native"] = bta_native->currentIndex() == 1 ? "2" : "1";
        currentConfig["bta_conf_allow"] = QString::number(bta_conf_allow->currentIndex() + 1);
        currentConfig["background"] = QString::number(background_mode->currentIndex() + 1);
        currentConfig["viewp_background"] = QString::number(viewp_background_mode->currentIndex() + 1);
        currentConfig["force_background_image"] = force_background_image->currentIndex() == 1 ? "2" : "1";
        currentConfig["custom_banner_css"] = custom_banner_css->currentIndex() == 1 ? "2" : "1";
        currentConfig["custom_banner_text"] = custom_banner_text->currentIndex() == 1 ? "2" : "1";

        // Style/Color Keys
        currentConfig["accent"] = accent_color->text();
        currentConfig["background_solid"] = background_solid->text();
        currentConfig["background_image"] = background_image->text();
        currentConfig["border"] = border_color->text();
        currentConfig["text"] = text_color->text();
        currentConfig["bta_css_color"] = bta_css_color->text();
        currentConfig["bta_css_curve"] = bta_css_curve->text();
        currentConfig["background_solid"] = background_solid->text();
        currentConfig["viewp_background_color"] = viewp_background_color->text();
        currentConfig["background_image"] = background_image->text();
        
        currentConfig["banner_css_color"] = banner_css_color->text();
        currentConfig["banner_css_border_color"] = banner_css_border->text();
        currentConfig["banner_css_theme"] = banner_css_theme->text();
        currentConfig["banner_css_light_inverted"] = banner_css_light_inverted->text();
        currentConfig["banner_css_force"] = banner_css_force->text();
        currentConfig["banner_text_force"] = banner_text_force->text();
    }
    
private:
    QListWidget *themeList;
    QTabWidget *editorTabs;
    QString currentTheme;
    QJsonObject currentConfig;

    QComboBox *theme_color_time, *theme_fusion_style_support, *force_fusion_win10;
    QComboBox *bta_css, *bta_color_css, *bta_native, *bta_conf_allow;
    QComboBox *background_mode, *viewp_background_mode, *force_background_image;
    QComboBox *custom_banner_css, *custom_banner_text;

    QLineEdit *text_color, *accent_color, *border_color;
    QLineEdit *bta_css_color, *bta_css_curve;
    QLineEdit *background_solid, *viewp_background_color, *background_image;
    QLineEdit *banner_css_color, *banner_css_border, *banner_css_theme;
    QLineEdit *banner_css_light_inverted, *banner_css_force, *banner_text_force;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ThemeMaker maker;
    maker.show();
    return app.exec();
}

#include "ThemeMakerCore.moc"
