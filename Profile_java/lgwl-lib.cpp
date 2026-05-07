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

// External logger interface
void LogLauncherEvent(const QString &message);

/**
 * lgwl-lib.cpp - Manages LWJGL downloads and paths for instances.
 */

// Forward declaration for internal use
void downloadAndExtractLwgl(const QString &url, const QString &name, const QString &ext, const QString &targetDir); // Defined below
QString GetLwglVersionForMc(const QString &mcVersion); // From lgwl-handlelib.cpp

QString GetLwglNativesPath(const QString &mcVersion) {
    QString lwglVersion = GetLwglVersionForMc(mcVersion);
    QString lwglBaseDir = QDir::current().absoluteFilePath(QString("Lib/lwjgl-%1").arg(lwglVersion));
    QString nativesDir = lwglBaseDir + "/natives";

    if (!QDir(nativesDir).exists()) {
        LogLauncherEvent("LWJGL natives for MC " + mcVersion + " (LWJGL " + lwglVersion + ") not found. Attempting to download...");
        QDir().mkpath(nativesDir);

        // Example LWJGL download URL (this should be dynamic based on OS/Arch and MC version)
        QString lwglDownloadUrl;
        QString os = (QSysInfo::kernelType() == "winnt") ? "windows" : "linux";
        QString arch = QSysInfo::currentCpuArchitecture();

        if (os == "windows") {
            lwglDownloadUrl = QString("https://build.lwjgl.org/release/%1/lwjgl-%1-natives-windows.zip").arg(lwglVersion);
        } else if (os == "linux") {
            lwglDownloadUrl = QString("https://build.lwjgl.org/release/%1/lwjgl-%1-natives-linux.tar.gz").arg(lwglVersion);
        } else {
            LogLauncherEvent("Unsupported OS for LWJGL download: " + os);
            return nativesDir; // Return path even if not downloaded, might be handled by game
        }

        QString extension = (os == "windows") ? ".zip" : ".tar.gz";
        downloadAndExtractLwgl(lwglDownloadUrl, "LWJGL-" + lwglVersion, extension, nativesDir);
    }

    return nativesDir;
}

// Global function: GetLwglArtifactLocalPath - Determines local path for LWJGL artifacts
QString GetLwglArtifactLocalPath(const QString &mcVersion, const QString &remoteUrl) {
    QString lwglVersion = GetLwglVersionForMc(mcVersion);
    QString lwglBaseDir = QDir::current().absoluteFilePath(QString("Lib/lwjgl-%1").arg(lwglVersion));
    
    QString fileName = remoteUrl.section('/', -1); // Extract filename from URL
    return lwglBaseDir + "/" + fileName;
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
#ifdef Q_OS_WIN
            process.start("powershell", {"-Command", QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(filePath, targetDir)});
#else
            process.start("tar", {"-xzf", filePath, "-C", targetDir, "--strip-components=1"});
#endif
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