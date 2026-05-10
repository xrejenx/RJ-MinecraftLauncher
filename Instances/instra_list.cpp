#include <QComboBox> // Changed from QListWidget
#include <QString>
#include <filesystem>
#include "Core.h"

namespace fs = std::filesystem;

/**
 * instra_list.cpp - Manages the listing of Minecraft instances from Instance_mc
 */

void PopulateInstanceList(QComboBox *comboBox) { // Changed from QListWidget *list
    if (!comboBox) return;
    comboBox->clear();
    
    std::string pathStr = MinecraftLauncher::getRJLDataPath().toStdString() + "Instances";
    if (!fs::exists(pathStr)) fs::create_directories(pathStr);

    for (const auto& entry : fs::directory_iterator(pathStr)) {
        if (entry.is_directory()) {
            comboBox->addItem(QString::fromStdString(entry.path().filename().string())); // Changed from list->addItem
        }
    }
}