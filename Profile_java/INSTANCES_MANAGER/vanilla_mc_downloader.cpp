// Profile_java/INSTANCES_MANAGER/vanilla_mc_downloader.cpp
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QProgressDialog>
#include <QEventLoop>
#include <QVariantMap>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QCoreApplication>

// External logger interface
void LogLauncherEvent(const QString &message);
void Insta_prefix_vanillamc_maker(const QString &path, const QString &version); // From vanilla_installer.cpp
void ShowJavaDownloadMenu(QWidget *parent = nullptr); // From Profile/Java_download.cpp
void AutoDownloadJavaHeadless(const QString &javaName, QProgressDialog *externalProgress = nullptr);
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0); // From instance-javaRequirement.cpp
QString GetAppName();
// Forward declarations for LWJGL functions (from Profile_java/lgwl-lib.cpp)
QString GetLwglNativesPath(const QString &mcVersion);
// Forward declarations for LWJGL artifact path and download (from Profile_java/lgwl-lib.cpp)
QString GetLwglArtifactLocalPath(const QString &mcVersion, const QString &remoteUrl);
void downloadAndExtractLwgl(const QString &url, const QString &name, const QString &ext, const QString &targetDir); // For internal use in lgwl-lib.cpp
/**
 * vanilla_mc_downloader.cpp - Handles asynchronous download of Minecraft JAR files.
 */

void DownloadVanillaInstance(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId) {
    // --- PHASE 0: JAVA COMPATIBILITY CHECK ---
    LogLauncherEvent("Initiating pre-download Java verification...");
    bool javaExists = false;
    QSettings s(QCoreApplication::applicationDirPath() + "/launcher.ini", QSettings::IniFormat);
    QString savedPath = s.value("java/path", "").toString();

    // Get centralized Java version requirement
    int metaMajor = metadata.contains("javaMajor") ? metadata["javaMajor"].toInt() : 0;
    int major = GetRequiredJavaMajorVersion(versionId, metaMajor);
    QString req = "jdk-" + QString::number(major);

    if (!savedPath.isEmpty() && savedPath != "java" && QFile::exists(savedPath)) {
        javaExists = true;
    } else {
        QDir javaDir("javas");
        if (javaDir.exists()) {
            QStringList subDirs = javaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &dir : subDirs) {
                if (dir.contains(req.section('-', -1))) { // Match major version
                    javaExists = true;
                    break;
                }
            }
        }
    }

    if (!javaExists) {
        LogLauncherEvent("Required Java (" + req + ") not found. Starting background download...");
        QProgressDialog javaProgress("Preparing Java environment...", "Cancel", 0, 100);
        javaProgress.setWindowTitle(GetAppName());
        javaProgress.setWindowModality(Qt::WindowModal);
        javaProgress.show();
        
        AutoDownloadJavaHeadless(req, &javaProgress);
        
        // Final check after download: Look for the specific version directory
        bool specificJavaFound = false;
        QDir checkDir("javas");
        for (const QString &d : checkDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (d.contains(QString::number(major))) {
                specificJavaFound = true;
                break;
            }
        }
        if (!specificJavaFound) {
            QMessageBox::critical(nullptr, "Java Error", QString("Could not install Java %1. The installation cannot continue.").arg(major));
            return;
        }
    }

    // --- PHASE 1: MINECRAFT DOWNLOAD ---
    QString clientUrl = metadata["clientUrl"].toString();
    QStringList libraries = metadata["libraries"].toStringList();
    QString assetIndexId = metadata["assetIndexId"].toString();
    QString assetIndexUrl = metadata["assetIndexUrl"].toString();
    QJsonArray assetObjects = metadata["assetObjects"].toJsonArray();
    
    int totalFiles = 1 + libraries.size() + 1 + assetObjects.size(); // Client JAR + Libraries + Asset Index JSON + Asset Objects
    QProgressDialog progress("Initializing installation...", "Cancel", 0, totalFiles);
    progress.setWindowTitle(GetAppName());
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    QSettings settings(QCoreApplication::applicationDirPath() + "/launcher.ini", QSettings::IniFormat);
    int maxParallel = settings.value("connection/parallel", 20).toInt();

    QNetworkAccessManager manager;
    QEventLoop loop;
    int activeDownloads = 0;
    int completedCount = 0;
    QStringList queue;
    QMap<QString, QString> urlToPath;

    // Prepare Queue
    QString jarName = QString("Minecraft %1.jar").arg(versionId);
    queue.append(clientUrl);
    urlToPath[clientUrl] = targetDirPath + "/" + jarName;

    QString instanceName = targetDirPath.section('/', -1);

    // Add Asset Index JSON
    if (!assetIndexUrl.isEmpty() && !assetIndexId.isEmpty()) {
        QString assetIndexPath = QDir::current().absoluteFilePath(QString("Assets/%1/indexes/%2.json").arg(instanceName, assetIndexId));
        queue.append(assetIndexUrl);
        urlToPath[assetIndexUrl] = assetIndexPath;
    }

    // Add individual asset objects
    for (const QJsonValue &val : assetObjects) {
        QJsonObject obj = val.toObject();
        QString url = obj["url"].toString();
        QString path = QDir::current().absoluteFilePath(QString("Assets/%1/%2").arg(instanceName, obj["path"].toString()));
        if (!url.isEmpty() && !path.isEmpty()) {
            queue.append(url);
            urlToPath[url] = path;
        }
    }

    // --- MODIFIED SECTION START ---
    // LWJGL is now managed by lgwl-lib.cpp directly when GetLwglNativesPath is called.
    // No need to add LWJGL artifacts to this download queue directly unless they are part of the main manifest.
    for (const QString &url : libraries) {
        queue.append(url);
        // --- MODIFIED SECTION START ---
        urlToPath[url] = GetLwglArtifactLocalPath(versionId, url);
        // --- MODIFIED SECTION END ---
    }

    auto startNext = [&]() {
        while (activeDownloads < maxParallel && !queue.isEmpty() && !progress.wasCanceled()) {
            QString url = queue.takeFirst();
            QString dest = urlToPath[url];
            QString label = dest.section('/', -1);
            
            // --- MODIFIED SECTION START --- (Connection URL and Next File Display) - Fix "Next: None"
            QString nextFileLabel = "None";
            if (!queue.isEmpty()) {
                QString nextUrlInQueue = queue.first(); // This is the URL, not the file name
                nextFileLabel = nextUrlInQueue.section('/', -1);
            }
            progress.setLabelText(QString("Connecting to: %1\nDownloading: %2\nNext: %3")
                                 .arg(url).arg(label).arg(nextFileLabel));
            // --- MODIFIED SECTION END ---


            activeDownloads++;
            QNetworkRequest request{QUrl(url)};
            QNetworkReply *reply = manager.get(request);
            
            QObject::connect(reply, &QNetworkReply::finished, [&, reply, dest, label, maxParallel]() {
                activeDownloads--;
                completedCount++;
                
                if (reply->error() == QNetworkReply::NoError) {
                    // Ensure target directory exists for the library
                    QDir().mkpath(QFileInfo(dest).path());
                    QFile file(dest);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(reply->readAll());
                        file.close();
                    }
                }
                
                reply->deleteLater();
                progress.setValue(completedCount);
                
                // Improved Next display
                QString nextFileLabelAfter;
                if (queue.isEmpty()) nextFileLabelAfter = (activeDownloads > 0) ? "Finishing active..." : "Done";
                else nextFileLabelAfter = queue.first().section('/', -1);

                progress.setLabelText(QString("Finished: %1\nStill Downloading: %2 files\nNext: %3")
                                     .arg(label).arg(activeDownloads).arg(nextFileLabelAfter)); // Show next file
                // --- MODIFIED SECTION END ---




                if (queue.isEmpty() && activeDownloads == 0) loop.quit();
                else {
                   // Recursively trigger next batch
                   // startNext() is called via signal loop
                }
            });
        }
        if (progress.wasCanceled() || (queue.isEmpty() && activeDownloads == 0)) loop.quit();
    };

    // We need a way to keep triggering startNext. 
    // Simplest for this logic is to call it inside the finished signal.
    // The connection below will ensure startNext is called when any download finishes.
    QObject::connect(&manager, &QNetworkAccessManager::finished, startNext);

    // Start initial batch
    QTimer::singleShot(0, startNext);

    loop.exec();


    // 3. Create Installer/Prefix info
    Insta_prefix_vanillamc_maker(targetDirPath, versionId);
    
    progress.setValue(totalFiles);
    LogLauncherEvent("Installation of " + versionId + " completed.");
}