#include <QComboBox> // Changed from QListWidget
#include <QString>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * instra_list.cpp - Manages the listing of Minecraft instances from Instance_mc
 */

void PopulateInstanceList(QComboBox *comboBox) { // Changed from QListWidget *list
    if (!comboBox) return;
    comboBox->clear();
    
    if (!fs::exists("Instances")) fs::create_directories("Instances");

    for (const auto& entry : fs::directory_iterator("Instances")) {
        if (entry.is_directory()) {
            comboBox->addItem(QString::fromStdString(entry.path().filename().string())); // Changed from list->addItem
        }
    }
}