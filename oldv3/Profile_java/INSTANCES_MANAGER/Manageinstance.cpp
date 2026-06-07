#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include "Core.h"

// Forward declarations
void ShowInstanceEditor(QWidget *parent, const QString &instanceName);
void ShowJVMDownloaderNew(QWidget *parent);
void ShowInstanceWizard(QWidget *parent);
void PopulateInstanceList(QListWidget *listWidget);

class ManageInstanceWindow : public QDialog {
    Q_OBJECT
public:
    explicit ManageInstanceWindow(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Instance manager");
        setFixedSize(700, 400);

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);
        
        // Top Section
        auto *contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(10);
        
        // Left Control Side
        auto *leftLayout = new QVBoxLayout();
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(10);
        auto *btnUp = new QPushButton("Move up", this);
        btnUp->setFixedWidth(120);
        auto *btnDown = new QPushButton("Movedown", this);
        btnDown->setFixedWidth(120);
        auto *btnDelete = new QPushButton("Delete", this);
        btnDelete->setFixedWidth(120);
        auto *btnOpen = new QPushButton("Open Instance", this);
        btnOpen->setFixedWidth(120);
        
        leftLayout->addWidget(btnUp);
        leftLayout->addWidget(btnDown);
        leftLayout->addWidget(btnDelete);
        leftLayout->addStretch();
        leftLayout->addWidget(btnOpen);
        
        // Center List
        instanceList = new QListWidget(this);
        PopulateInstanceList(instanceList);
        
        // Right Side
        auto *rightLayout = new QVBoxLayout();
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(10);
        auto *btnAdd = new QPushButton("Add Minecraft Instance", this);
        btnAdd->setFixedWidth(150);
        auto *btnJava = new QPushButton("Java Manager", this);
        btnJava->setFixedWidth(150);
        auto *btnFolder = new QPushButton("Open Instance Folder", this);
        btnFolder->setFixedWidth(150);
        
        rightLayout->addWidget(btnAdd);
        rightLayout->addWidget(btnJava);
        rightLayout->addWidget(btnFolder);
        rightLayout->addStretch();

        contentLayout->addLayout(leftLayout);
        contentLayout->addWidget(instanceList, 1);
        contentLayout->addLayout(rightLayout);

        mainLayout->addLayout(contentLayout);

        // Bottom Section: Ok Button
        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        auto *btnOk = new QPushButton("Ok", this);
        btnOk->setFixedWidth(80);
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnOk);
        mainLayout->addLayout(bottomLayout);

        // Connects
        connect(btnAdd, &QPushButton::clicked, this, [this](){ ShowInstanceWizard(this); PopulateInstanceList(instanceList); });
        connect(btnJava, &QPushButton::clicked, this, [this](){ ShowJVMDownloaderNew(this); });
        connect(btnFolder, &QPushButton::clicked, this, [this](){
            QDesktopServices::openUrl(QUrl::fromLocalFile(MinecraftLauncher::getRJLDataPath() + "Instances"));
        });
        connect(btnOpen, &QPushButton::clicked, this, &ManageInstanceWindow::openEditor);
        connect(instanceList, &QListWidget::itemDoubleClicked, this, &ManageInstanceWindow::openEditor);
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        
        connect(btnDelete, &QPushButton::clicked, this, [this](){
            if (!instanceList->currentItem()) return;
            QString name = instanceList->currentItem()->text();
            QDir(MinecraftLauncher::getRJLDataPath() + "Instances/" + name).removeRecursively();
            PopulateInstanceList(instanceList);
        });
    }

private slots:
    void openEditor() {
        if (auto *item = instanceList->currentItem()) {
            ShowInstanceEditor(this, item->text());
        }
    }

private:
    QListWidget *instanceList;
};

void ShowManageInstanceWindow(QWidget *parent) {
    ManageInstanceWindow dlg(parent);
    dlg.exec();
}

#include "Manageinstance.moc"