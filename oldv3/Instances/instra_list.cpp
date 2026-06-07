#include <QComboBox> // Changed from QListWidget
#include <QListWidget>
#include <QString>
#include <QDir>
#include "Core.h"

/**
 * instra_list.cpp - Manages the listing of Minecraft instances from Instance_mc
 */

void PopulateInstanceList(QComboBox *comboBox) { // Changed from QListWidget *list
    if (!comboBox) return;
    comboBox->clear();
    QString path = MinecraftLauncher::getRJLDataPath() + "Instances";
    if (!QDir(path).exists()) QDir().mkpath(path);
    comboBox->addItems(QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name));
}

void PopulateInstanceList(QListWidget *listWidget) {
    if (!listWidget) return;
    listWidget->clear();
    QString path = MinecraftLauncher::getRJLDataPath() + "Instances";
    if (!QDir(path).exists()) QDir().mkpath(path);
    listWidget->addItems(QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name));
}
