#include "instmc.h"
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, LogLauncherEvent
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QUrl>
#include <QListWidgetItem>
#include <QCryptographicHash>
#include <QApplication> // For QApplication::processEvents()
#include "JVM/downloadqueuedialog.h" // Include the new download queue dialog

// Forward declarations for LWJGL functions (from Profile_java/lwjgl-lib.cpp)
QString GetLwglArtifactLocalPath(const QString &mcVersion, const QString &remoteUrl);
QString GetLwglVersionForMc(const QString &mcVersion);

extern void LogLauncherEvent(const QString &message);
QString GetAppName();

// Helper function to verify file integrity via SHA1 (Prism-style)
bool verifyFileHash(const QString &filePath, const QString &expectedHash) {
    if (expectedHash.isEmpty()) return true;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (hash.addData(&file)) {
        return hash.result().toHex().toLower() == expectedHash.toLower();
    }
    return false;
}


InstMC::InstMC(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)) // Keep network manager for metadata fetching if needed
{
    QSettings settings(MinecraftLauncher::getRJLDataPath() + "launcher.ini", QSettings::IniFormat); //
    m_maxParallelDownloads = settings.value("connection/parallel", 20).toInt(); //
}

bool InstMC::downloadMinecraftAssets(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId, QWidget *parentWidget) {
    LogLauncherEvent("InstMC::downloadMinecraftAssets: Starting Minecraft asset download for " + versionId); //

    QString dataRoot = MinecraftLauncher::getRJLDataPath(); //

    QString clientUrl = metadata["clientUrl"].toString(); //
    QVariantList libraries = metadata["libraries"].toList();
    QString installerUrl = metadata["installerUrl"].toString();
    QString selectedLoader = metadata["loader"].toString();
    QVariantList natives = metadata["natives"].toList();
    QString assetIndexId = metadata["assetIndexId"].toString(); //
    QString assetIndexUrl = metadata["assetIndexUrl"].toString(); //
    QJsonArray assetObjects = metadata["assetObjects"].toJsonArray(); //
    
    QList<QListWidgetItem*> downloadItemsForQueue; // List to hold items for the DownloadQueueDialog

    // Prepare Queue
    QString jarName = QString("Minecraft %1.jar").arg(versionId); //
    QString jarPath = targetDirPath + "/" + jarName;
    if (!QFile::exists(jarPath)) {
        QListWidgetItem *jarItem = new QListWidgetItem(); 
        jarItem->setText(selectedLoader == "Vanilla" ? "Minecraft JAR" : "Minecraft Base JAR");
        jarItem->setData(Qt::UserRole, clientUrl); //
        jarItem->setData(Qt::UserRole + 1, jarPath); //
        downloadItemsForQueue.append(jarItem); //
    }

    // Download specific Modloader JAR files (Fabric/Quilt/Forge) instead of just Vanilla
    if (selectedLoader != "Vanilla") {
        for (const QVariant &libVar : libraries) {
            QString url = libVar.toMap()["url"].toString();
            if (url.contains(selectedLoader.toLower() + "-loader") || (url.contains("forge") && url.contains("-universal")) || url.contains("neoforge")) {
                QListWidgetItem *loaderItem = new QListWidgetItem(selectedLoader + " Loader JAR");
                loaderItem->setData(Qt::UserRole, url);
                loaderItem->setData(Qt::UserRole + 1, targetDirPath + "/" + QUrl(url).fileName());
                downloadItemsForQueue.append(loaderItem);
            }
        }
    }

    // Add Modloader Installer if present (Forge/NeoForge)
    if (!installerUrl.isEmpty()) {
        QString installerPath = targetDirPath + "/" + selectedLoader.toLower() + "-installer.jar";
        QListWidgetItem *instItem = new QListWidgetItem();
        instItem->setText(selectedLoader + " Installer JAR");
        instItem->setData(Qt::UserRole, installerUrl);
        instItem->setData(Qt::UserRole + 1, installerPath);
        downloadItemsForQueue.append(instItem);
    }

    QString instanceName = targetDirPath.section('/', -1); //
    if (instanceName.isEmpty()) instanceName = "Default";

    // Add Asset Index JSON
    if (!assetIndexUrl.isEmpty() && !assetIndexId.isEmpty()) { //
        // Store indexes in a shared directory
        QString assetIndexPath = dataRoot + QString("Assets/indexes/%1.json").arg(assetIndexId);
        QDir().mkpath(dataRoot + "Assets/indexes");
        if (!QFile::exists(assetIndexPath)) {
            QListWidgetItem *assetIndexItem = new QListWidgetItem("Asset Index (" + assetIndexId + ")"); //
            assetIndexItem->setData(Qt::UserRole, assetIndexUrl); //
            assetIndexItem->setData(Qt::UserRole + 1, assetIndexPath); //
            downloadItemsForQueue.append(assetIndexItem); //
        }
    }

    // Add individual asset objects
    for (const QJsonValue &val : assetObjects) { //
        QJsonObject obj = val.toObject(); //
        QString url = obj["url"].toString(); //
        // Global assets folder for deduplication
        QString relPath = obj["path"].toString();
        QString expectedHash = obj["hash"].toString();
        QString path = dataRoot + "Assets/" + relPath;

        bool skip = false;
        if (QFile::exists(path)) {
            // Perform Hash Check
            if (verifyFileHash(path, expectedHash)) {
                skip = true;
            } else {
                LogLauncherEvent("Asset corrupted, scheduling redownload: " + relPath);
                QFile::remove(path);
            }
        }

        if (!url.isEmpty() && !path.isEmpty() && !skip) {
            QListWidgetItem *assetItem = new QListWidgetItem("Asset: " + QFileInfo(path).fileName()); //
            assetItem->setData(Qt::UserRole, url); //
            assetItem->setData(Qt::UserRole + 1, path); //
            downloadItemsForQueue.append(assetItem); //
        }
    }

    // Add libraries
    for (const QVariant &libVal : libraries) {
        QVariantMap libObj = libVal.toMap();
        QString url = libObj["url"].toString();
        QString relPath = libObj["path"].toString();

        QString libPath;
        // Skip LWJGL Core jars for Phase 3, but KEEP paulscode bridges in Lib/ for sound
        if ((relPath.contains("org/lwjgl") || relPath.contains("jinput")) && !relPath.contains("paulscode")) {
            continue; // Skip LWJGL in Phase 2 to handle it in the isolated Phase 3
        }
        libPath = dataRoot + "Lib/" + relPath;

        if (!QFile::exists(libPath)) {
            QDir().mkpath(QFileInfo(libPath).absolutePath());
            QListWidgetItem *libItem = new QListWidgetItem("Library: " + QFileInfo(libPath).fileName()); //
            libItem->setData(Qt::UserRole, url); //
            libItem->setData(Qt::UserRole + 1, libPath); //
            downloadItemsForQueue.append(libItem); //
        }
    }

    // Ensure icons directory exists to prevent ImageIO crash in legacy versions
    QString iconDir = dataRoot + "Assets/icons/";
    QDir().mkpath(iconDir);

    // Provide fallback files to satisfy Launchwrapper's VanillaTweakInjector
    // This prevents the IIOException without needing an external download
    if (!QFile::exists(iconDir + "icon_16x16.png")) QFile::copy(":/launcher/icons/CmakeLauncherIcon/release/icon.png", iconDir + "icon_16x16.png");
    if (!QFile::exists(iconDir + "icon_32x32.png")) QFile::copy(":/launcher/icons/CmakeLauncherIcon/release/icon.png", iconDir + "icon_32x32.png");

    // Create legacy resources folder in instance dir to satisfy the ThreadDownloadResources check
    QDir(targetDirPath + "/resources").mkpath(".");

    m_instanceNativesDir = targetDirPath + "/natives"; //

    // If everything is already downloaded, finish immediately
    if (downloadItemsForQueue.isEmpty()) {
        LogLauncherEvent("InstMC: All files already present. Skipping download phase.");
        emit minecraftDownloadFinished(true);
        return true;
    }

    LogLauncherEvent(QString("InstMC: %1 files need to be downloaded.").arg(downloadItemsForQueue.size()));

    // Now, use the DownloadQueueDialog for all these files
    DownloadQueueDialog *queueDialog = new DownloadQueueDialog(downloadItemsForQueue, parentWidget); //
    QString title = GetAppName() + " - Downloading ";
    if (selectedLoader != "Vanilla") title += selectedLoader + " " + versionId;
    else title += "Minecraft " + versionId;
    queueDialog->setWindowTitle(title); //
    int result = queueDialog->exec(); // Show as modal dialog //

    bool success = (result == QDialog::Accepted); //
    if (success) { //
        LogLauncherEvent("InstMC::downloadMinecraftAssets: All Minecraft assets downloaded successfully for " + versionId); //

        // Phase 1.5: Legacy Asset Reconstruction (Sound Fix for 1.0 - 1.5.2)
        if (assetIndexId == "pre-1.6") {
            LogLauncherEvent("InstMC: Reconstructing legacy resources for " + versionId);
            for (const QJsonValue &val : assetObjects) {
                QJsonObject obj = val.toObject();
                QString hashedPath = dataRoot + "Assets/" + obj["path"].toString();
                QString legacyPath = targetDirPath + "/resources/" + obj["legacyPath"].toString();
                
                if (QFile::exists(hashedPath) && !QFile::exists(legacyPath)) {
                    QDir().mkpath(QFileInfo(legacyPath).absolutePath());
                    QFile::copy(hashedPath, legacyPath);
                }
            }
        }

    } else {
        LogLauncherEvent("InstMC::downloadMinecraftAssets: Minecraft asset download for " + versionId + " was cancelled or failed."); //
    }

    // Clean up the QListWidgetItems created for the queue dialog
    for (QListWidgetItem* item : downloadItemsForQueue) { //
        delete item; //
    }
    queueDialog->deleteLater(); //
    emit minecraftDownloadFinished(success); //
    return success; //
}
