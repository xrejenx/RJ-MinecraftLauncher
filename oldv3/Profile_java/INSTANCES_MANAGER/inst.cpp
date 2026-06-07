#include "inst.h"
#include "jvmdownloadernew.h" // For JavaVersionItemWidget
#include "JVM/downloadqueuedialog.h"
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, LogLauncherEvent
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QEventLoop>
#include <QSysInfo>
#include <QDir>
#include <QTimer>

extern void LogLauncherEvent(const QString &message);
QString GetAppName(); // From Core.cpp

Inst::Inst(QObject *parent) : QObject(parent) {}

QJsonObject Inst::findJavaPackage(int javaMajorVersion, const QString &os, const QString &arch) {
    QString apiUrl = QString("https://api.azul.com/metadata/v1/zulu/packages/?os=%1&arch=%2&java_version=%3&java_package_type=jdk&latest=true&release_status=ga") //
                        .arg(os, arch, QString::number(javaMajorVersion)); //

    QNetworkAccessManager manager; //
    QEventLoop loop; //
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(apiUrl))); //
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit); //
    loop.exec(); //

    QJsonObject result; //
    if (reply->error() == QNetworkReply::NoError) { //
        QJsonArray packages = QJsonDocument::fromJson(reply->readAll()).array(); //
        if (!packages.isEmpty()) { //
            result = packages.first().toObject(); //
        }
    } else {
        LogLauncherEvent("Failed to fetch Java package info for Java " + QString::number(javaMajorVersion) + ": " + reply->errorString()); //
    }
    reply->deleteLater(); //
    return result; //
}

bool Inst::downloadJavaForInstance(const QString &versionId, int javaMajorVersion, QWidget *parentWidget) {
    LogLauncherEvent("Inst::downloadJavaForInstance: Checking for Java " + QString::number(javaMajorVersion)); //

    QString dataRoot = MinecraftLauncher::getRJLDataPath(); //
    QDir javasDir(dataRoot + "javas"); //
    if (javasDir.exists()) { //
        QStringList subDirs = javasDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot); //
        for (const QString &dir : subDirs) { //
            QString majorStr = QString::number(javaMajorVersion);
            // Fix: Strict matching. 'dir.contains' was matching 'jdk-18' for 'Java 8'.
            // We now check for the specific zulu/jdk prefix followed by the version and a boundary.
            if (dir.startsWith("zulu" + majorStr + ".") || dir.startsWith("zulu-jdk-" + majorStr) || 
                dir.startsWith("jdk-" + majorStr + ".") || dir == "java" + majorStr) {
                QString binName = "java"; //
#ifdef Q_OS_WIN
                binName = "java.exe"; //
#endif
                QString detectedPath = dataRoot + "javas/" + dir + "/bin/" + binName; //
                if (QFile::exists(detectedPath)) { //
                    LogLauncherEvent("Inst::downloadJavaForInstance: Java " + QString::number(javaMajorVersion) + " already installed at " + detectedPath); //
                    emit javaDownloadFinished(true, detectedPath); //
                    return true; // Java already exists //
                }
            }
        }
    }

    LogLauncherEvent("Inst::downloadJavaForInstance: Java " + QString::number(javaMajorVersion) + " not found. Initiating download."); //

    QString os = (QSysInfo::kernelType() == "winnt") ? "windows" : "linux"; //
    QString arch = (QSysInfo::currentCpuArchitecture() == "x86_64") ? "x64" : QSysInfo::currentCpuArchitecture(); //
    QJsonObject pkg = findJavaPackage(javaMajorVersion, os, arch); //

    if (pkg.isEmpty()) { //
        QMessageBox::critical(parentWidget, "Java Download Error", "Could not find a suitable Java " + QString::number(javaMajorVersion) + " package for your system."); //
        emit javaDownloadFinished(false, ""); //
        return false; //
    }

    QString downloadUrl = pkg["download_url"].toString(); //
    QString javaName = QString("zulu-jdk-%1").arg(pkg["java_version"].toArray().first().toVariant().toString()); //
    QString ext = (QSysInfo::kernelType() == "winnt") ? ".zip" : ".tar.gz"; //

    // Create a dummy QListWidgetItem to pass to DownloadQueueDialog
    // This is a bit of a hack, but reuses the existing DownloadQueueDialog structure
    QListWidgetItem *dummyItem = new QListWidgetItem(); //
    dummyItem->setText(javaName);
    dummyItem->setData(Qt::UserRole, downloadUrl); //
    dummyItem->setData(Qt::UserRole + 1, QVariant::fromValue(pkg)); //

    // Create a temporary JavaVersionItemWidget for the DownloadQueueDialog to display
    // This widget won't be added to a QListWidget, but its properties are used.
    JavaVersionItemWidget *tempItemWidget = new JavaVersionItemWidget(javaName, nullptr); //
    tempItemWidget->listItem = dummyItem; // Link to dummy item //
    tempItemWidget->checkBox->setChecked(true); // Mark as checked for download //

    QList<QListWidgetItem*> itemsToDownload; //
    itemsToDownload.append(dummyItem); // The DownloadQueueDialog will take ownership of this item //

    DownloadQueueDialog *queueDialog = new DownloadQueueDialog(itemsToDownload, parentWidget); //
    queueDialog->setWindowTitle("Downloading Java " + QString::number(javaMajorVersion)); //
    int result = queueDialog->exec(); // Show as modal dialog //

    bool success = (result == QDialog::Accepted); //
    QString finalJavaPath = ""; //

    if (success) { //
        // After successful download, verify installation path
        QDir checkDir(dataRoot + "javas"); //
        QString majorStr = QString::number(javaMajorVersion);
        for (const QString &d : checkDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) { //
            // Use strict matching to avoid 'jdk8' matching '17' due to sub-version numbers
            if (d.startsWith("zulu" + majorStr + ".") || d.startsWith("zulu-jdk-" + majorStr) || 
                d.startsWith("jdk-" + majorStr + ".") || d == "java" + majorStr) {
                
                QString binName = "java"; //
#ifdef Q_OS_WIN
                binName = "java.exe"; //
#endif
                QString detectedPath = dataRoot + "javas/" + d + "/bin/" + binName; //
                if (QFile::exists(detectedPath)) { //
                    finalJavaPath = detectedPath; //
                    break; //
                }
            }
        }
        if (finalJavaPath.isEmpty()) { //
            QMessageBox::critical(parentWidget, "Java Installation Error", "Java " + QString::number(javaMajorVersion) + " was downloaded but could not be located."); //
            success = false; //
        }
    } else {
        QMessageBox::warning(parentWidget, "Java Download Cancelled", "Java " + QString::number(javaMajorVersion) + " download was cancelled or failed."); //
    }

    // The DownloadQueueDialog takes ownership of dummyItem and tempItemWidget, so no need to delete here.
    // delete dummyItem; // Clean up dummy item //
    // delete tempItemWidget; // Clean up temporary widget //
    queueDialog->deleteLater(); //

    emit javaDownloadFinished(success, finalJavaPath); //
    return success; //
}
