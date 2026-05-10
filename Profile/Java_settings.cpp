#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QComboBox>
#include <QDir>
#include <QSpinBox>
#include <QTabWidget>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include "Core.h"

void ShowJavaDownloadMenu(QWidget *parent);
void LogLauncherEvent(const QString &message);
QString GetLauncherTitle();

class JavaSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit JavaSettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(GetLauncherTitle() + " - Java Settings");
        setFixedSize(500, 350);
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        auto *mainLayout = new QVBoxLayout(this);
        QSettings settings(dataRoot + "launcher.ini", QSettings::IniFormat);

        auto *tabs = new QTabWidget(this);

        // --- Tab 1: General ---
        QWidget *generalTab = new QWidget();
        QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);

        generalLayout->addWidget(new QLabel("<b>Select Java Runtime:</b>"));
        auto *pathLayout = new QHBoxLayout();
        javaPathCombo = new QComboBox(this);
        auto *dlBtn = new QPushButton("Download Java...");
        
        refreshJavaList(settings.value("java/path", "java").toString());
        
        pathLayout->addWidget(javaPathCombo, 1);
        pathLayout->addWidget(dlBtn);
        generalLayout->addLayout(pathLayout);

        generalLayout->addWidget(new QLabel("<b>Max RAM Allocation (GB):</b>"));
        auto *ramSpin = new QSpinBox();
        ramSpin->setRange(1, 64);
        ramSpin->setValue(settings.value("java/ram", 2).toInt());
        generalLayout->addWidget(ramSpin);
        generalLayout->addStretch();

        tabs->addTab(generalTab, "General");

        // --- Tab 2: Java Tweaks ---
        QWidget *tweaksTab = new QWidget();
        QVBoxLayout *tweaksLayout = new QVBoxLayout(tweaksTab);

        tweaksLayout->addWidget(new QLabel("<b>Custom Path Management:</b>"));
        QHBoxLayout *tweaksRow = new QHBoxLayout();
        pathEdit = new QLineEdit(this);
        pathEdit->setPlaceholderText("Select custom java path...");
        QPushButton *detectBtn = new QPushButton("Auto-Detect", this);
        QPushButton *testBtn = new QPushButton("Test", this);
        tweaksRow->addWidget(pathEdit);
        tweaksRow->addWidget(detectBtn);
        tweaksRow->addWidget(testBtn);
        tweaksLayout->addLayout(tweaksRow);

        QPushButton *openFolderBtn = new QPushButton("Open Java Folder", this);
        tweaksLayout->addStretch();
        tweaksLayout->addWidget(openFolderBtn);

        tabs->addTab(tweaksTab, "Java Tweaks");

        // --- Tab 3: Theme Settings ---
        QWidget *themeSettingsTab = new QWidget();
        QVBoxLayout *themeSettingsLayout = new QVBoxLayout(themeSettingsTab);
        autoThemeCheck = new QCheckBox("Disable Auto Theme Detection", this);
        autoThemeCheck->setChecked(settings.value("theme/disableAutoColor", false).toBool());
        themeSettingsLayout->addWidget(autoThemeCheck);
        themeSettingsLayout->addStretch();
        tabs->addTab(themeSettingsTab, "Theme Settings");

        mainLayout->addWidget(tabs);

        auto *saveBtn = new QPushButton("Save");
        mainLayout->addWidget(saveBtn);

        connect(dlBtn, &QPushButton::clicked, this, [this]() {
            ShowJavaDownloadMenu(this);
            refreshJavaList();
        });

        connect(openFolderBtn, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(MinecraftLauncher::getRJLDataPath() + "javas"));
        });

        connect(detectBtn, &QPushButton::clicked, this, [this]() {
            QDir javaDir(MinecraftLauncher::getRJLDataPath() + "javas");
            QStringList subDirs = javaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            
            if (subDirs.isEmpty()) {
                QMessageBox::information(this, "Auto-Detect", "No Java versions found in the local javas folder.");
                return;
            }

            bool ok;
            QString selectedDir = QInputDialog::getItem(this, "Auto-Detect Java", 
                                                       "Select a Java version found in the javas folder:", 
                                                       subDirs, 0, false, &ok);
            
            if (ok && !selectedDir.isEmpty()) {
                QString dirPath = MinecraftLauncher::getRJLDataPath() + "javas/" + selectedDir;
                QString binPath;
#ifdef Q_OS_WIN
                binPath = QDir(dirPath).absoluteFilePath("bin/java.exe");
#else
                binPath = QDir(dirPath).absoluteFilePath("bin/java");
#endif
                if (QFile::exists(binPath)) {
                    pathEdit->setText(binPath);
                    refreshJavaList(binPath);
                    LogLauncherEvent("Java detected and set: " + binPath);
                } else {
                    QMessageBox::warning(this, "Detection Error", "Could not find a valid java executable in " + selectedDir);
                }
            }
        });

        connect(saveBtn, &QPushButton::clicked, this, [this, ramSpin]() {
            QSettings s(MinecraftLauncher::getRJLDataPath() + "launcher.ini", QSettings::IniFormat);
            s.setValue("java/path", javaPathCombo->currentData().toString());
            s.setValue("java/ram", ramSpin->value());
            s.setValue("theme/disableAutoColor", autoThemeCheck->isChecked());
            this->accept();
        });
    }

private:
    QComboBox *javaPathCombo;
    QLineEdit *pathEdit;
    QCheckBox *autoThemeCheck;

    void refreshJavaList(const QString &currentSavedPath = "") {
        javaPathCombo->clear();
        
        // 1. Add System Default
        javaPathCombo->addItem("System Default (java)", "java");

        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        // 2. Scan javas directory for custom installs
        QDir javaDir(dataRoot + "javas");
        if (javaDir.exists()) {
            QStringList subDirs = javaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &dir : subDirs) {
                QString binPath;
#ifdef Q_OS_WIN
                binPath = QDir(dataRoot + "javas/" + dir).absoluteFilePath("bin/java.exe");
#else
                binPath = QDir(dataRoot + "javas/" + dir).absoluteFilePath("bin/java");
#endif
                if (QFile::exists(binPath)) {
                    javaPathCombo->addItem(dir, binPath);
                }
            }
        }

        // Set current selection
        int index = javaPathCombo->findData(currentSavedPath);
        if (index != -1) javaPathCombo->setCurrentIndex(index);
    }
};

void ShowJavaSettingsWindow(QWidget *parent) {
    JavaSettingsDialog dlg(parent);
    dlg.exec();
}

#include "Java_settings.moc"