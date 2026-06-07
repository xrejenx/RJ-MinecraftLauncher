#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <filesystem>
#include "JavaProfile.h"

// Forward declarations for external UI functions
void ShowInstanceWizard(QWidget *parent);
void ShowInstanceSettings(QWidget *parent, const QString &instanceName);
void ShowJavaSettingsWindow(QWidget *parent);
void PopulateInstanceList(QListWidget *listWidget);

namespace fs = std::filesystem;

/** 
Implementation of JavaProfileWindow (JavaProfile logic)
 */
JavaProfileWindow::JavaProfileWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Minecraft Instances");
    auto *layout = new QVBoxLayout(this);
    auto *header = new QLabel("<h2>Minecraft Instances</h2>", this);
    layout->addWidget(header);

    auto *mainArea = new QHBoxLayout();
    instanceList = new QListWidget(this);
    refreshInstances();
    mainArea->addWidget(instanceList, 1);

    QVBoxLayout *sideButtons = new QVBoxLayout();
    QPushButton *addBtn = new QPushButton("Add Minecraft Instance", this);
    QPushButton *removeBtn = new QPushButton("Remove Instance", this);
    QPushButton *manageBtn = new QPushButton("Manage Instance", this);
    QPushButton *javaBtn = new QPushButton("Java Settings", this);

    sideButtons->addWidget(addBtn);
    sideButtons->addWidget(removeBtn);
    sideButtons->addWidget(manageBtn);
    sideButtons->addWidget(javaBtn);
    sideButtons->addStretch();

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        ShowInstanceWizard(this);
        refreshInstances();
    });

    connect(manageBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = instanceList->currentItem();
        if (!item) return;
        ShowInstanceSettings(this, item->text());
    });

    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = instanceList->currentItem();
        if (!item) return;

        auto reply = QMessageBox::question(this, "Remove Instance", 
                                          "Are you sure you want to delete " + item->text() + "?",
                                          QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            fs::remove_all("Instances/" + item->text().toStdString());
            refreshInstances();
        }
    });

    connect(javaBtn, &QPushButton::clicked, this, [this]() {
        ShowJavaSettingsWindow(this);
    });

    mainArea->addLayout(sideButtons);
    layout->addLayout(mainArea);

    QPushButton *closeBtn = new QPushButton("Done", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}

JavaProfileWindow::~JavaProfileWindow() {}

void JavaProfileWindow::refreshInstances() {
    instanceList->clear();
    if (!fs::exists("Instances")) {
        fs::create_directories("Instances");
        return;
    }

    for (const auto& entry : fs::directory_iterator("Instances")) {
        if (entry.is_directory()) {
            instanceList->addItem(QString::fromStdString(entry.path().filename().string()));
        }
    }
}

/**
 * Global function to show the window
 */
void ShowJavaProfileWindow(QWidget *parent) {
    JavaProfileWindow dialog(parent);
    dialog.exec();
}
