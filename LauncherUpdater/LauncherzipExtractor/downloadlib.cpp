#include <QString>
#include "LauncherUpdater/LauncherDownload/download.h" // For DownloadFileToPath
#include "Core.h"

// Using fetchos to determine which helper library to download
namespace FetchOS { QString getPlatformName(); }

/**
 * downloadlib.cpp - Downloads the OS-specific extraction binary (e.g. 7zip-standalone).
 */
void SyncExtractionLibrary() {
    QString os = FetchOS::getPlatformName();
    QString libUrl;
    QString destinationFileName;

    if (os == "windows") {
        libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Windows7zip/7za.exe";
        destinationFileName = "7za.exe";
    } else {
        libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Linux7zip/7za";
        destinationFileName = "7za";
    }

    DownloadFileToPath(libUrl, MinecraftLauncher::getRJLDataPath() + "Lib/" + destinationFileName);
}