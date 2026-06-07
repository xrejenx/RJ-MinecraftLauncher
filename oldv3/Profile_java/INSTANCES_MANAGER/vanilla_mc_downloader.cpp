// Profile_java/INSTANCES_MANAGER/vanilla_mc_downloader.cpp
#include <QString>
#include <QNetworkAccessManager>
#include <QFile>
#include <QProgressBar>
#include <QProgressDialog>
#include <QEventLoop>
#include <QVariantMap>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QTimer> // For QTimer::singleShot
#include "Core.h"
#include "inst.h" // New Java download step
#include "instmc.h" // New Minecraft assets download step
#include "instlwjgl.h" // New LWJGL download step
#include "instcreate.h" // New finalization step

// External logger interface
void LogLauncherEvent(const QString &message);
void Insta_prefix_vanillamc_maker(const QString &path, const QString &version); // From vanilla_installer.cpp
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0); // From instance-javaRequirement.cpp
QString GetAppName();
// Forward declarations for LWJGL functions (from Profile_java/lwjgl-lib.cpp)
QString GetLwglVersionForMc(const QString &mcVersion);
QString GetLwglNativesPath(const QString &mcVersion);
/**
 * vanilla_mc_downloader.cpp - Handles asynchronous download of Minecraft JAR files.
 */

void DownloadVanillaInstance(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId) {
    QString loader = metadata.value("loader", "Vanilla").toString();
    LogLauncherEvent(QString("DownloadVanillaInstance: Starting %1 instance creation flow for %2").arg(loader, versionId)); //

    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    QString instanceName = targetDirPath.section('/', -1); //

    // Get centralized Java version requirement
    int metaMajor = metadata.contains("javaMajor") ? metadata["javaMajor"].toInt() : 0; //
    int major = GetRequiredJavaMajorVersion(versionId, metaMajor); //

    // --- PHASE 1: MINECRAFT ASSETS DOWNLOAD ---
    LogLauncherEvent("DownloadVanillaInstance: Phase 1 - Minecraft Assets Download for " + versionId); //
    InstMC mcDownloader; //
    bool mcDownloadSuccess = mcDownloader.downloadMinecraftAssets(metadata, targetDirPath, versionId, nullptr); // Pass nullptr for parent widget //
    if (!mcDownloadSuccess) { //
        QMessageBox::critical(nullptr, "Instance Creation Failed", "Minecraft assets download failed or was cancelled. Cannot proceed."); //
        return; //
    }

    // --- PHASE 2: ISOLATED LWJGL DOWNLOAD ---
    LogLauncherEvent("DownloadVanillaInstance: Phase 2 - Isolated LWJGL Phase for " + versionId);
    InstLWJGL lwjglDownloader;
    bool lwjglDownloadSuccess = lwjglDownloader.downloadLWJGL(metadata, targetDirPath, nullptr);
    if (!lwjglDownloadSuccess) { //
        QMessageBox::critical(nullptr, "Instance Creation Failed", "LWJGL download failed or was cancelled. Cannot proceed."); //
        return; //
    }

    // --- PHASE 3: JAVA DOWNLOAD ---
    LogLauncherEvent("DownloadVanillaInstance: Phase 3 - Java Check/Download for " + versionId); //
    Inst javaDownloader; //
    QString javaPath = ""; //
    bool javaDownloadSuccess = javaDownloader.downloadJavaForInstance(versionId, major, nullptr); // Pass nullptr for parent widget for now, or a specific parent if available //

    if (!javaDownloadSuccess) { //
        QMessageBox::critical(nullptr, "Instance Creation Failed", "Java download failed or was cancelled. Cannot proceed with instance creation."); //
        return; //
    }
    // After successful check or download, retrieve the actual path
    QDir javasDir(dataRoot + "javas"); //
    for (const QString &d : javasDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) { //
        QString majorStr = QString::number(major);
        // Fix: Strict matching to prevent Java 8 being detected as Java 17/18 due to sub-version strings
        if (d.startsWith("zulu" + majorStr + ".") || 
            d.startsWith("zulu-jdk-" + majorStr) || 
            d.startsWith("jdk-" + majorStr + ".") || 
            d == "java" + majorStr) {
            QString binName = "java"; //
#ifdef Q_OS_WIN
            binName = "java.exe"; //
#endif
            QString detectedPath = dataRoot + "javas/" + d + "/bin/" + binName; //
            if (QFile::exists(detectedPath)) { //
                javaPath = detectedPath; //
                break; //
            }
        }
    }
    if (javaPath.isEmpty()) { //
        QMessageBox::critical(nullptr, "Instance Creation Failed", "Java was downloaded but its executable could not be found. Cannot proceed."); //
        return; //
    }

    // --- PHASE 4: FINALIZE INSTANCE ---
    LogLauncherEvent("DownloadVanillaInstance: Phase 4 - Finalizing instance " + versionId); //
    InstCreate instanceCreator; //
    // The instanceCreator will emit a signal that the main window can connect to.
    // For now, this is a direct call.
    instanceCreator.finalizeInstance(instanceName, versionId, javaPath, mcDownloadSuccess, lwjglDownloadSuccess, nullptr); // Pass nullptr for parent widget //

    LogLauncherEvent("DownloadVanillaInstance: Instance creation flow completed for " + versionId); //
}
