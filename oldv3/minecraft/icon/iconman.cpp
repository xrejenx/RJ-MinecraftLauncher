#include "iconman.h"
#include "../../Core.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QDirIterator>
#include <QCoreApplication>
#include <QListWidgetItem>
#include <QIcon>
#include <QFileInfo>

IconManager::IconManager(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Icon Manager");
    setFixedSize(500, 400);

    auto *mainLayout = new QVBoxLayout(this);
    
    iconList = new QListWidget(this);
    iconList->setViewMode(QListWidget::IconMode);
    iconList->setIconSize(QSize(64, 64));
    iconList->setResizeMode(QListWidget::Adjust);
    iconList->setSpacing(10);
    mainLayout->addWidget(iconList);

    auto *bottomLayout = new QHBoxLayout();
    auto *btnSelect = new QPushButton("Select", this);
    auto *btnCancel = new QPushButton("Cancel", this);
    auto *btnOpen = new QPushButton("Open Folder", this);
    totalLabel = new QLabel("Total: 0 icons", this);

    bottomLayout->addWidget(btnSelect);
    bottomLayout->addWidget(btnCancel);
    bottomLayout->addWidget(btnOpen);
    bottomLayout->addStretch();
    bottomLayout->addWidget(totalLabel);
    mainLayout->addLayout(bottomLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSelect, &QPushButton::clicked, this, [this]() {
        if (auto *item = iconList->currentItem()) {
            m_selectedPath = item->data(Qt::UserRole).toString();
            accept();
        }
    });
    connect(btnOpen, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(MinecraftLauncher::getRJLDataPath() + "icons"));
    });

    scanIcons();
    populateList();
}

void IconManager::scanIcons() {
    m_iconSets.clear();
    QString iconsPath = MinecraftLauncher::getRJLDataPath() + "icons/rjiconset";
    
    // Recursive search to handle nested directories in rjiconset
    QDirIterator it(iconsPath + "/png", {"*.png"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString pngPath = it.filePath();
        QString baseName = it.fileInfo().completeBaseName();
        
        // Try to find matching ico recursively in ico folder
        QDirIterator itIco(iconsPath + "/ico", {baseName + ".ico"}, QDir::Files, QDirIterator::Subdirectories);
        if (itIco.hasNext()) {
            m_iconSets.append({baseName, pngPath, itIco.next()});
        }
    }
}

void IconManager::populateList() {
    iconList->clear();
    for (const auto &set : m_iconSets) {
        auto *item = new QListWidgetItem(set.name, iconList);
        item->setIcon(QIcon(set.pngPath));
        item->setData(Qt::UserRole, set.icoPath);
    }
    // Updated label to show the count of matched icon sets
    totalLabel->setText(QString("Total: %1 %2").arg(QString::number(m_iconSets.size()), (m_iconSets.size() == 1 ? "icon" : "icons")));
}

QString IconManager::getSelectedIconPath() const {
    return m_selectedPath;
}

void SyncIconsToData() {
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QString src = QCoreApplication::applicationDirPath() + "/minecraft/icon";
    QString dst = dataRoot + "icons";
    
    if (QDir(src).exists()) {
        QDir().mkpath(dst);
        // Recursive copy implementation
        QDirIterator it(src, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QString relPath = it.filePath().mid(src.length());
            if (it.fileInfo().isDir()) {
                QDir().mkpath(dst + relPath);
            } else {
                QFile::copy(it.filePath(), dst + relPath);
            }
        }
    }
}
