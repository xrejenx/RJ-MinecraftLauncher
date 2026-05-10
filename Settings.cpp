#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QSettings>
#include <QWidget>
#include <QCoreApplication>
#include "Core.h"

/**
 * Internal Class Declarations (Headerless)
 */

// External function interfaces
void EnsureConsoleVisibility();
void LogLauncherEvent(const QString &message);

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        if (parent) setStyleSheet(parent->styleSheet());
        setWindowTitle("Settings");
        setFixedSize(450, 350);

        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QSettings settings(dataRoot + "launcher.ini", QSettings::IniFormat);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QTabWidget *tabWidget = new QTabWidget(this);

        // General Tab
        QWidget *generalTab = new QWidget();
        QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
        
        QLabel *debugHeader = new QLabel("<b>Debug Options</b>", this);
        QCheckBox *debugToggle = new QCheckBox("Enable Launcher Debug Mode", this);
        debugToggle->setToolTip("Toggle to see internal launcher logs and developer tools.");
        
        connect(debugToggle, &QCheckBox::toggled, this, [](bool checked) {
            LogLauncherEvent(QString("Debug mode %1").arg(checked ? "Enabled" : "Disabled"));
            EnsureConsoleVisibility();
        });

        generalLayout->addWidget(debugHeader);
        generalLayout->addWidget(debugToggle);

#ifdef Q_OS_WIN
        QCheckBox *cmdToggle = new QCheckBox("Enable cmds", this);
        cmdToggle->setChecked(settings.value("launcher/showConsole", false).toBool());
        connect(cmdToggle, &QCheckBox::toggled, this, [](bool checked) {
            QString dataRoot = MinecraftLauncher::getRJLDataPath();
            QSettings s(dataRoot + "launcher.ini", QSettings::IniFormat);
            s.setValue("launcher/showConsole", checked);
            EnsureConsoleVisibility();
        });
        generalLayout->addWidget(cmdToggle);
#endif

        generalLayout->addStretch();

        // Connection Tab
        QWidget *connectionTab = new QWidget();
        QVBoxLayout *connLayout = new QVBoxLayout(connectionTab);
        connLayout->addWidget(new QLabel("<b>Download Settings</b>", this));

        QHBoxLayout *parallelLayout = new QHBoxLayout();
        parallelLayout->addWidget(new QLabel("Parallel Downloads:", this));
        QSpinBox *parallelSpin = new QSpinBox(this);
        parallelSpin->setRange(1, 100);
        parallelSpin->setValue(settings.value("connection/parallel", 20).toInt());
        parallelLayout->addWidget(parallelSpin);
        connLayout->addLayout(parallelLayout);

        connect(parallelSpin, qOverload<int>(&QSpinBox::valueChanged), [](int val) {
            QString dataRoot = MinecraftLauncher::getRJLDataPath();
            QSettings(dataRoot + "launcher.ini", QSettings::IniFormat).setValue("connection/parallel", val);
        });

        connLayout->addStretch();

        tabWidget->addTab(generalTab, "General");
        tabWidget->addTab(connectionTab, "Connection");
        mainLayout->addWidget(tabWidget);

        QPushButton *doneBtn = new QPushButton("Done", this);
        connect(doneBtn, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(doneBtn, 0, Qt::AlignRight);
    }
};

void ShowSettingsWindow(QWidget *parent) {
    SettingsDialog dialog(parent);
    dialog.exec();
}

#include "Settings.moc"