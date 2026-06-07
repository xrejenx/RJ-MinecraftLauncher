#include "instlwjgl.h"
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, LogLauncherEvent
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <QEventLoop>
#include <QSysInfo>
#include <QJsonDocument>
#include <QProgressDialog>
#include <QTimer>
#include <QApplication> // For QApplication::processEvents()
#include <QUrl>
#include <QListWidgetItem>
#include "JVM/downloadqueuedialog.h"

// Forward declarations for LWJGL functions (from Profile_java/lwjgl-lib.cpp)
QString GetLwglVersionForMc(const QString &mcVersion);
QString GetLwglNativesPath(const QString &mcVersion);
QString GetLwglArtifactLocalPath(const QString &mcVersion, const QString &remoteUrl);

extern void LogLauncherEvent(const QString &message);
QString GetAppName();

// Helper to construct Maven URL
QString getMavenUrl(const QString& groupId, const QString& artifactId, const QString& version, const QString& classifier = "", const QString& extension = "jar") {
    QString groupIdPath = groupId;
    groupIdPath.replace('.', '/');
    QString url = QString("https://repo1.maven.org/maven2/%1/%2/%3/%2-%3").arg(groupIdPath, artifactId, version);
    if (!classifier.isEmpty()) {
        url += "-" + classifier;
    }
    url += "." + extension;
    return url;
}

InstLWJGL::InstLWJGL(QObject *parent) : QObject(parent) {}

bool InstLWJGL::downloadLWJGL(const QVariantMap &metadata, const QString &targetDirPath, QWidget *parentWidget) {
    QString mcVersion = metadata.value("version", "unknown").toString();
    LogLauncherEvent("InstLWJGL: Initiating isolated LWJGL phase for MC " + mcVersion);

    QString dataRoot = MinecraftLauncher::getRJLDataPath(); //
    int lwMajor = metadata.value("lwjglMajor", 3).toInt();
    QString lwglVer = metadata.value("lwjglVersion", "3.3.1").toString();
    
    QString lwglBaseDir = dataRoot + QString("Lib/LWJGL/Minecraft/v%1/%2").arg(QString::number(lwMajor), lwglVer);
    QString absoluteNativesPath = lwglBaseDir + "/natives";

    // Check if LWJGL is already present
    QDir lwjglDir(lwglBaseDir); //
    if (lwjglDir.exists() && !lwjglDir.entryList(QDir::Files).isEmpty()) { //
        LogLauncherEvent("InstLWJGL::downloadLWJGL: LWJGL " + lwglVer + " already installed."); //
        emit lwjglDownloadFinished(true); //
        return true; //
    }

    QDir().mkpath(lwglBaseDir);
    QDir().mkpath(absoluteNativesPath);

    QList<QListWidgetItem*> downloadQueue;
    QVariantList libraries = metadata["libraries"].toList();
    QVariantList natives = metadata["natives"].toList();

    for (const QVariant &v : libraries) {
        QString url = v.toMap()["url"].toString();
        // Include jinput as it's required for LWJGL 2.x input handling
        if (url.contains("lwjgl") || url.contains("jinput")) {
            QString dest = lwglBaseDir + "/" + QUrl(url).fileName();
            if (!QFile::exists(dest)) {
                QListWidgetItem *item = new QListWidgetItem("LWJGL Library: " + QFileInfo(dest).fileName());
                item->setData(Qt::UserRole, url);
                item->setData(Qt::UserRole + 1, dest);
                downloadQueue.append(item);
            }
        }
    }

    for (const QVariant &v : natives) {
        QString url = v.toMap()["url"].toString();
        if (url.contains("lwjgl") || url.contains("jinput")) {
            QString dest = absoluteNativesPath + "/" + QUrl(url).fileName();
            if (!QFile::exists(dest)) {
                QListWidgetItem *item = new QListWidgetItem("LWJGL Native: " + QFileInfo(dest).fileName());
                item->setData(Qt::UserRole, url);
                item->setData(Qt::UserRole + 1, dest);
                downloadQueue.append(item);
            }
        }
    }

    // Legacy Fix: Manually ensure Paulscode Sound Bridges are present for LWJGL 2
    if (lwMajor == 2) {
        QString bridgeUrl = "https://libraries.minecraft.net/com/paulscode/librarylwjglopenal/20100824/librarylwjglopenal-20100824.jar";
        QString bridgeDest = lwglBaseDir + "/librarylwjglopenal-20100824.jar";
        if (!QFile::exists(bridgeDest)) {
            QListWidgetItem *bridgeItem = new QListWidgetItem("LWJGL Sound Bridge");
            bridgeItem->setData(Qt::UserRole, bridgeUrl);
            bridgeItem->setData(Qt::UserRole + 1, bridgeDest);
            downloadQueue.append(bridgeItem);
        }
    }

    if (downloadQueue.isEmpty()) return true;

    DownloadQueueDialog *queueDialog = new DownloadQueueDialog(downloadQueue, parentWidget);
    queueDialog->setWindowTitle(GetAppName() + " - LWJGL Dependencies");
    int result = queueDialog->exec();

    bool success = (result == QDialog::Accepted);
    emit lwjglDownloadFinished(success);
    return success;
}
