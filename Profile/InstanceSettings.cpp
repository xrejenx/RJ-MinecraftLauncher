#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include "Core.h"

class InstanceSettingsDialog : public QDialog {
    Q_OBJECT
public:
    InstanceSettingsDialog(QWidget *parent, const QString &instanceName) 
        : QDialog(parent), m_instanceName(instanceName) {
        setWindowTitle("Instance Settings: " + instanceName);
        setFixedSize(550, 450);
        if (parent) setStyleSheet(parent->styleSheet());

        auto *layout = new QVBoxLayout(this);
        auto *tabs = new QTabWidget(this);

        // Main Tab
        auto *mainTab = new QWidget();
        auto *mainLayout = new QVBoxLayout(mainTab);
        QString dataRoot = MinecraftLauncher::getRJLDataPath();

        mainLayout->addWidget(new QLabel("<b>Launch Arguments:</b>"));
        argsEdit = new QTextEdit();
        
        m_path = dataRoot + "Instances/" + instanceName + "/args.txt";
        QFile file(m_path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            argsEdit->setPlainText(file.readAll());
            file.close();
        }
        mainLayout->addWidget(argsEdit);

        auto *btnLayout = new QHBoxLayout();
        auto *saveBtn = new QPushButton("Save");
        btnLayout->addStretch();
        btnLayout->addWidget(saveBtn);
        mainLayout->addLayout(btnLayout);

        connect(saveBtn, &QPushButton::clicked, this, [this]() {
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(argsEdit->toPlainText().toUtf8());
                file.close();
            }
            accept();
        });

        tabs->addTab(mainTab, "Main");
        layout->addWidget(tabs);
    }
private:
    QString m_instanceName;
    QString m_path;
    QTextEdit *argsEdit;
};

void ShowInstanceSettings(QWidget *parent, const QString &instanceName) {
    InstanceSettingsDialog dlg(parent, instanceName);
    dlg.exec();
}
#include "InstanceSettings.moc"