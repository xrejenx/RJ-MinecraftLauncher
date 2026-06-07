#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSettings>
#include "../../minecraft/icon/iconman.h"
#include "Core.h"

class InstanceEditor : public QDialog {
    Q_OBJECT
public:
    InstanceEditor(const QString &instanceName, QWidget *parent = nullptr) 
        : QDialog(parent), m_name(instanceName) {
        setWindowTitle("Instance: " + instanceName);
        setFixedSize(850, 500);

        loadInstanceConfig();

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);

        // Middle Section
        auto *centerLayout = new QHBoxLayout();
        centerLayout->setSpacing(10);

        sidebar = new QListWidget(this);
        sidebar->setFixedWidth(150);
        QStringList sidebarItems = {"Version"};
        if (m_config.value("Modloader", "Vanilla") != "Vanilla") {
            sidebarItems << "Mods";
        }
        sidebarItems << QStringList{"Visual Packs", "Worlds & Saves", "Screenshots", "Servers", "Settings", "Notes"};
        sidebar->addItems(sidebarItems);
        
        pages = new QStackedWidget(this);
        setupPages();

        centerLayout->addWidget(sidebar);
        centerLayout->addWidget(pages, 1);

        mainLayout->addLayout(centerLayout);

        // Bottom Section: Ok Button
        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        auto *btnOk = new QPushButton("Ok", this);
        btnOk->setFixedWidth(80);
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnOk);
        mainLayout->addLayout(bottomLayout);

        connect(sidebar, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
        connect(btnOk, &QPushButton::clicked, this, [this](){
            saveInstanceConfig();
            accept();
        });
    }

private:
    void saveInstanceConfig() {
        QString path = MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/instance.txt";
        QSettings settings(path, QSettings::IniFormat);
        
        settings.beginGroup("PRODUCT");
        settings.setValue("Instancename", m_name);
        settings.setValue("Minecraft", m_config["Minecraft"]);
        settings.setValue("Java requirements", m_config["Java requirements"]);
        settings.setValue("LWJGL", m_config["LWJGL"]);
        settings.setValue("Group", m_config["Group"]);
        settings.endGroup();

        settings.beginGroup("Settings");
        settings.setValue("Minimum Ram", m_config["Minimum Ram"]);
        settings.setValue("Maximum Ram", m_config["Maximum Ram"]);
        settings.setValue("Permgen", m_config["Permgen"]);
        settings.endGroup();

        settings.beginGroup("Tweaks");
        settings.setValue("JVM Args", m_config["JVM Args"]);
        settings.setValue("Window Width", m_config["Window Width"]);
        settings.setValue("Window Height", m_config["Window Height"]);
        settings.endGroup();

        settings.sync();
    }

    void loadInstanceConfig() {
        QString path = MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/instance.txt";
        QSettings settings(path, QSettings::IniFormat);
        for (const QString &group : settings.childGroups()) {
            settings.beginGroup(group);
            for (const QString &key : settings.allKeys()) {
                m_config[key] = settings.value(key).toString();
            }
            settings.endGroup();
        }
    }

    void setupPages() {
        // Page 0: Version (Combined Hub)
        QWidget *pVersion = new QWidget();
        auto *lVersion = new QVBoxLayout(pVersion);
        lVersion->setContentsMargins(0, 0, 0, 0);
        lVersion->setSpacing(10);

        QHBoxLayout *nameEditBox = new QHBoxLayout();
        
        // Square Icon Box
        QPushButton *iconBtn = new QPushButton(pVersion);
        iconBtn->setFixedSize(64, 64);
        QString localIconPath = MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/icon/icon.png";
        if (QFile::exists(localIconPath)) iconBtn->setIcon(QIcon(localIconPath));
        else iconBtn->setIcon(QIcon(":/CmakeLauncherIcon/release/icon.png"));
        iconBtn->setIconSize(QSize(48, 48));
        
        // Group Selection
        QVBoxLayout *vNaming = new QVBoxLayout();
        QHBoxLayout *groupLay = new QHBoxLayout();
        groupLay->addWidget(new QLabel("Group:"));
        QComboBox *groupCombo = new QComboBox(pVersion);
        QSettings lS(MinecraftLauncher::getRJLDataPath() + "launcher.ini", QSettings::IniFormat);
        groupCombo->addItems(lS.value("launcher/groups", QStringList{"Default"}).toStringList());
        groupCombo->setEditable(true);
        groupCombo->setCurrentText(m_config.value("Group", "Default"));
        groupLay->addWidget(groupCombo, 1);
        connect(groupCombo, &QComboBox::currentTextChanged, [this](const QString &text){ m_config["Group"] = text; });

        connect(iconBtn, &QPushButton::clicked, this, [this, iconBtn]() {
            IconManager dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                QString icoPath = dlg.getSelectedIconPath();
                QString pngPath = icoPath;
                pngPath.replace("/ico/", "/png/").replace(".ico", ".png");

                QString instIconDir = MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/icon";
                QDir().mkpath(instIconDir);

                // Copy and rename to local instance folder
                QFile::remove(instIconDir + "/icon.ico");
                QFile::remove(instIconDir + "/icon.png");
                QFile::copy(icoPath, instIconDir + "/icon.ico");
                QFile::copy(pngPath, instIconDir + "/icon.png");

                m_config["Icon"] = instIconDir + "/icon.ico";
                iconBtn->setIcon(QIcon(instIconDir + "/icon.png"));
            }
        });

        nameEditBox->addWidget(iconBtn);
        vNaming->addWidget(new QLabel("Instance Name:"));
        QLineEdit *nameEdit = new QLineEdit(m_name);
        vNaming->addWidget(nameEdit);
        vNaming->addLayout(groupLay);
        nameEditBox->addLayout(vNaming, 1);
        connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &text){ m_name = text; });
        nameEditBox->addWidget(new QPushButton("✎"));
        lVersion->addLayout(nameEditBox);

        QGroupBox *compGroup = new QGroupBox("Components", pVersion);
        QVBoxLayout *compLay = new QVBoxLayout(compGroup);
        QListWidget *compList = new QListWidget(compGroup);
        compList->addItem("Minecraft: " + m_config.value("Minecraft", "Unknown"));
        compList->addItem("LWJGL: " + m_config.value("LWJGL", "Unknown"));
        compList->addItem("Java Requirement: Java " + m_config.value("Java requirements", "8"));
        compList->addItem("Modloader: " + m_config.value("Modloader", "Vanilla"));
        compLay->addWidget(compList);

        QHBoxLayout *loaderBtns = new QHBoxLayout();
        loaderBtns->addStretch();
        loaderBtns->addWidget(new QPushButton("Edit JSON"));
        compLay->addLayout(loaderBtns);

        lVersion->addWidget(compGroup);
        lVersion->addStretch();
        pages->addWidget(pVersion);

        // Page 1: Mods
        if (m_config.value("Modloader", "Vanilla") != "Vanilla") {
            QWidget *pMods = new QWidget();
            auto *lMods = new QHBoxLayout(pMods);
            lMods->setContentsMargins(0, 0, 0, 0);
            lMods->setSpacing(10);

            auto *vModControls = new QVBoxLayout();
            vModControls->setSpacing(10);
            vModControls->addWidget(new QPushButton("MoveUp"));
            vModControls->addWidget(new QPushButton("MoveDown"));
            vModControls->addWidget(new QPushButton("Delete"));
            vModControls->addWidget(new QPushButton("Import jar"));
            vModControls->addStretch();

            auto *btnOpenMods = new QPushButton("Open Folder");
            connect(btnOpenMods, &QPushButton::clicked, this, [this](){
                QDesktopServices::openUrl(QUrl::fromLocalFile(MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/mods"));
            });
            vModControls->addWidget(btnOpenMods);

            auto *vModList = new QVBoxLayout();
            vModList->addWidget(new QLabel("Modloader: " + m_config.value("Modloader", "Vanilla")));
            vModList->addWidget(new QListWidget(), 1);

            auto *vModMeta = new QVBoxLayout();
            vModMeta->setSpacing(10);
            vModMeta->addWidget(new QPushButton("Download mod"));
            vModMeta->addWidget(new QPushButton("Select all"));
            vModMeta->addStretch();

            lMods->addLayout(vModControls);
            lMods->addLayout(vModList, 1);
            lMods->addLayout(vModMeta);
            pages->addWidget(pMods);
        }

        // Page 2: Visual Packs (Resource + Shaders)
        QWidget *pVisual = new QWidget();
        auto *lVisual = new QHBoxLayout(pVisual);
        lVisual->setContentsMargins(0, 0, 0, 0);
        lVisual->setSpacing(10);
        
        auto *vTypeSidebar = new QVBoxLayout();
        vTypeSidebar->setSpacing(10);
        auto *packTypeSelector = new QListWidget();
        packTypeSelector->setFixedWidth(150);
        packTypeSelector->addItems({"Resource Packs", "Shader Packs"});
        // Fix height to fit 2 items exactly
        packTypeSelector->setFixedHeight(64);
        packTypeSelector->setCurrentRow(0);
        
        auto *vControlBtns = new QVBoxLayout();
        vControlBtns->setSpacing(10);
        auto *btnDelete = new QPushButton("Delete");
        btnDelete->setFixedWidth(150);
        auto *btnMoveUp = new QPushButton("Moveup");
        btnMoveUp->setFixedWidth(150);
        auto *btnMoveDown = new QPushButton("Movedown");
        btnMoveDown->setFixedWidth(150);
        vControlBtns->addWidget(btnDelete);
        vControlBtns->addWidget(btnMoveUp);
        vControlBtns->addWidget(btnMoveDown);

        vTypeSidebar->addWidget(packTypeSelector);
        vTypeSidebar->addLayout(vControlBtns);
        vTypeSidebar->addStretch(); // Push everything to the top

        auto *packList = new QListWidget();
        auto *btnDownload = new QPushButton("Download");
        btnDownload->setFixedWidth(100);

        lVisual->addLayout(vTypeSidebar, 0); 
        lVisual->addWidget(packList, 1);
        lVisual->addWidget(btnDownload, 0, Qt::AlignTop);

        pages->addWidget(pVisual);

        // Page 3: Worlds & Saves
        pages->addWidget(createListWithSidebar("Worlds & Saves", "saves"));
        // Page 4: Screenshots
        pages->addWidget(createListWithSidebar("Screenshots", "ScreenShort"));
        // Page 5: Servers
        pages->addWidget(createListWithSidebar("Servers", "")); 

        // Page 6: Settings (Instance Override)
        QWidget *pSettings = new QWidget();
        auto *lSettings = new QVBoxLayout(pSettings);
        lSettings->setContentsMargins(0, 0, 0, 0);
        lSettings->setSpacing(10);
        
        QGroupBox *javaGrp = new QGroupBox("Java & Memory", pSettings);
        QFormLayout *fJava = new QFormLayout(javaGrp);

        QLineEdit *minRam = new QLineEdit(m_config.value("Minimum Ram", "512M"), pSettings);
        QLineEdit *maxRam = new QLineEdit(m_config.value("Maximum Ram", "2G"), pSettings);
        QLineEdit *permGen = new QLineEdit(m_config.value("Permgen", "128M"), pSettings);
        QLineEdit *jvmArgs = new QLineEdit(m_config.value("JVM Args"), pSettings);

        connect(minRam, &QLineEdit::textChanged, [this](const QString &t){ m_config["Minimum Ram"] = t; });
        connect(maxRam, &QLineEdit::textChanged, [this](const QString &t){ m_config["Maximum Ram"] = t; });
        connect(permGen, &QLineEdit::textChanged, [this](const QString &t){ m_config["Permgen"] = t; });
        connect(jvmArgs, &QLineEdit::textChanged, [this](const QString &t){ m_config["JVM Args"] = t; });

        fJava->addRow("Minimum RAM:", minRam);
        fJava->addRow("Maximum RAM:", maxRam);
        fJava->addRow("PermGen (Max):", permGen);
        fJava->addRow("Custom JVM Args:", jvmArgs);

        lSettings->addWidget(javaGrp);

        QGroupBox *mcGrp = new QGroupBox("Minecraft Settings", pSettings);
        QFormLayout *fMc = new QFormLayout(mcGrp);

        QSpinBox *wWidth = new QSpinBox(pSettings); wWidth->setRange(0, 8000);
        wWidth->setValue(m_config.value("Window Width", "854").toInt());
        QSpinBox *wHeight = new QSpinBox(pSettings); wHeight->setRange(0, 8000);
        wHeight->setValue(m_config.value("Window Height", "480").toInt());

        connect(wWidth, qOverload<int>(&QSpinBox::valueChanged), [this](int v){ m_config["Window Width"] = QString::number(v); });
        connect(wHeight, qOverload<int>(&QSpinBox::valueChanged), [this](int v){ m_config["Window Height"] = QString::number(v); });

        fMc->addRow("Window Width:", wWidth);
        fMc->addRow("Window Height:", wHeight);
        fMc->addRow(new QCheckBox("Close launcher after launch", pSettings));
        lSettings->addWidget(mcGrp);

        lSettings->addStretch();
        pages->addWidget(pSettings);

        // Page 7: Notes
        QWidget *pNotes = new QWidget();
        auto *lNotes = new QVBoxLayout(pNotes);
        lNotes->setContentsMargins(0, 0, 0, 0);
        QPlainTextEdit *noteArea = new QPlainTextEdit(pNotes);
        noteArea->setPlaceholderText("Write down your coordinates, credits, or todo list here...");
        lNotes->addWidget(noteArea);
        pages->addWidget(pNotes);
    }

    QWidget* createListWithSidebar(const QString &title, const QString &subFolder) {
        QWidget *w = new QWidget();
        auto *l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(10);
        auto *btns = new QVBoxLayout();
        btns->setContentsMargins(0, 0, 0, 0);
        btns->setSpacing(10);
        auto *btnMU = new QPushButton("MoveUp");
        btnMU->setFixedWidth(120);
        auto *btnMD = new QPushButton("MoveDown");
        btnMD->setFixedWidth(120);
        auto *btnDel = new QPushButton("Delete");
        btnDel->setFixedWidth(120);
        btns->addWidget(btnMU);
        btns->addWidget(btnMD);
        btns->addWidget(btnDel);
        btns->addStretch(); // Push primary buttons to top

        if (!subFolder.isEmpty()) {
            auto *btnOpen = new QPushButton("Open Folder", w);
            btnOpen->setFixedWidth(120);
            connect(btnOpen, &QPushButton::clicked, this, [this, subFolder](){
                QDesktopServices::openUrl(QUrl::fromLocalFile(MinecraftLauncher::getRJLDataPath() + "Instances/" + m_name + "/" + subFolder));
            });
            btns->addWidget(btnOpen);
        }
        l->addLayout(btns);
        l->addWidget(new QListWidget(), 1);
        return w;
    }

    QString m_name;
    QMap<QString, QString> m_config;
    QListWidget *sidebar;
    QStackedWidget *pages;
};

void ShowInstanceEditor(QWidget *parent, const QString &instanceName) {
    InstanceEditor dlg(instanceName, parent);
    dlg.exec();
}

#include "insman.moc"
