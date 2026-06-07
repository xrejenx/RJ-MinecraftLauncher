#include <QString>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QProgressDialog>
#include <QSysInfo>
#include <QMessageBox>
#include "Core.h"

// External logger interface
void LogLauncherEvent(const QString &message);

/**
 * lwjgl-lib.cpp - Manages LWJGL downloads and paths for instances.
 */
QString GetLwglVersionForMc(const QString &mcVersion); // From lwjglhandle.cpp

QString GetLwglNativesPath(const QString &mcVersion) {
    if (mcVersion.isEmpty()) return QString();

    QString lwglVersion = GetLwglVersionForMc(mcVersion);

    bool isLegacy = (mcVersion.startsWith("1.12") || mcVersion.contains("beta") || mcVersion.contains("alpha") || mcVersion.startsWith("1.0"));
    if (isLegacy && lwglVersion.startsWith("3")) lwglVersion = "2.9.4";

    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    // Priority 1: Instance-specific natives (Downloaded dynamically from version manifest)
    QString nativesDir = dataRoot + "Instances/" + mcVersion + "/natives";

    if (!QDir(nativesDir).exists()) {
        LogLauncherEvent("LWJGL natives for MC " + mcVersion + " not found in instance. Falling back to global Lib.");
        // Priority 2: Shared version-specific Lib path
        nativesDir = dataRoot + "Lib/lwjgl-" + lwglVersion + "/natives";
        QDir().mkpath(nativesDir);
    }

    return nativesDir;
}

// Global function: GetLwglArtifactLocalPath - Determines local path for LWJGL artifacts
QString GetLwglArtifactLocalPath(const QString &mcVersion, const QString &remoteUrl) {
    QString fileName = remoteUrl.section('/', -1);
    QString dataRoot = MinecraftLauncher::getRJLDataPath();
    
    // Per user request: Put LWJGL and all MC library components into Lib/lwjgl-<version>
    QString lwglVersion = GetLwglVersionForMc(mcVersion);

    bool isLegacy = (mcVersion.startsWith("1.12") || mcVersion.contains("beta") || mcVersion.contains("alpha") || mcVersion.startsWith("1.0"));
    if (isLegacy && lwglVersion.startsWith("3")) lwglVersion = "2.9.4";

    QString targetDir = dataRoot + "Lib/lwjgl-" + lwglVersion;
    QDir().mkpath(targetDir);
    
    return targetDir + "/" + fileName;
}

// Global function: downloadAndExtractLwgl - Downloads and extracts LWJGL
void downloadAndExtractLwgl(const QString &url, const QString &name, const QString &ext, const QString &targetDir) {
    LogLauncherEvent("Starting LWJGL download from: " + url);
    QDir().mkpath(targetDir); // Ensure target directory exists

    QString fileName = name.simplified().replace(" ", "_") + ext;
    QString filePath = QDir(targetDir).absoluteFilePath(fileName);

    QProgressDialog progress("Downloading LWJGL...", "Cancel", 0, 100, nullptr); // Parent is nullptr for global dialog
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::downloadProgress, [&](qint64 r, qint64 t) {
        if (t > 0) progress.setValue((r * 100) / t);
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            progress.setLabelText("Extracting LWJGL...");
            
            QProcess process;
            QString sevenZipPath = MinecraftLauncher::getRJLDataPath() + "Lib/" + 
#ifdef Q_OS_WIN
                "7za.exe";
#else
                "7za";
#endif

            if (QFile::exists(sevenZipPath)) {
                process.start(sevenZipPath, {"x", filePath, "-o" + targetDir, "-y"});
            } else {
#ifdef Q_OS_WIN
                process.start("powershell", {"-Command", QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(filePath, targetDir)});
#else
                process.start("tar", {"-xzf", filePath, "-C", targetDir, "--strip-components=1"});
#endif
            }
            process.waitForFinished();
            file.remove(); // Cleanup downloaded archive
            LogLauncherEvent("LWJGL installed to: " + targetDir);
        } else {
            LogLauncherEvent("Failed to open file for writing LWJGL: " + filePath);
        }
    } else {
        LogLauncherEvent("LWJGL download failed: " + reply->errorString());
    }
    reply->deleteLater();
}