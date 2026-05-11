#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QProgressDialog>
#include <QCheckBox>
#include <QSysInfo>
#include <QMessageBox>
#include <QEventLoop>
#include <QUrl>
#include <QDir> // QDir needs to be included
#include <QJsonObject>
#include <QJsonDocument> // Added for QJsonDocument
#include <QJsonArray>    // Added for QJsonArray
#include "Core.h"
void LogLauncherEvent(const QString &message);
QString GetLauncherTitle();

/**
 * Java_download.cpp - Java Runtime Downloader
 */

// Forward declaration for LogLauncherEvent

class JavaDownloadDialog : public QDialog {
    Q_OBJECT
public:
    explicit JavaDownloadDialog(QWidget *parent = nullptr, bool loadData = true) : QDialog(parent), networkManager(new QNetworkAccessManager(this)) {
        setWindowTitle(GetLauncherTitle() + " - Download Java Runtime");
        setFixedSize(700, 550);
        auto *layout = new QVBoxLayout(this);
        
        // Filter Options
        auto *filterLayout = new QHBoxLayout();
        cbLTS = new QCheckBox("LTS Only", this); // Checkbox for LTS versions
        cbAllJava = new QCheckBox("Show all Java", this); // Renamed from cbOld
        cbLTS->setChecked(true);
        filterLayout->addWidget(new QLabel("Filters:"));
        filterLayout->addWidget(cbLTS);
        filterLayout->addWidget(cbAllJava);
        filterLayout->addStretch();
        layout->addLayout(filterLayout);

        layout->addWidget(new QLabel("<b>Available Java Versions:</b>"));

        tabs = new QTabWidget(this);

        // Tab 1: Azul Zulu
        azulList = new QListWidget(this);
        azulList->setStyleSheet("border: 1px solid #ccc;");
        tabs->addTab(azulList, "Azul Zulu");

        auto *dlBtn = new QPushButton("Download and Install Selected", this);
        layout->addWidget(dlBtn);

        connect(dlBtn, &QPushButton::clicked, this, &JavaDownloadDialog::onDownloadClicked);
        connect(cbLTS, &QCheckBox::toggled, this, [this](){ fetchAll(); }); 
        connect(cbAllJava, &QCheckBox::toggled, this, [this](){ fetchAll(); }); // Refresh on filter change
        
        if (loadData) fetchAll();
    }
    ~JavaDownloadDialog() override {}

    void fetchAll() {
        fetchAzulVersions();
    }

    // Public static method to allow auto-downloading from the installer
    static void AutoDownloadJava(const QString &javaName, QProgressDialog *externalProgress = nullptr);

private:
    QCheckBox *cbLTS, *cbAllJava; 
    QTabWidget *tabs;
    QListWidget *azulList;
    QNetworkAccessManager *networkManager;
    QJsonArray allJavaVersions;

    // Helper to get the current selected list widget
    QListWidget* currentSelectedList() {
        if (tabs->currentIndex() == 0) return azulList;
        return nullptr;
    }

    void fetchAzulVersions() {
        QString os = (QSysInfo::kernelType() == "winnt") ? "windows" : "linux";
        // Simplify filter: remove archive_type to ensure results appear
        QString ltsFilter = cbLTS->isChecked() ? "&support_term=lts" : "";
        QString latestFilter = cbAllJava->isChecked() ? "" : "&latest=true";
        QString gaFilter = "&release_status=ga";

        // Corrected URL formatting for Azul
        QUrl url(QString("https://api.azul.com/metadata/v1/zulu/packages/?os=%1&arch=x64&java_package_type=jdk%2%3%4")
                 .arg(os, ltsFilter, latestFilter, gaFilter));
                 
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::UserAgentHeader, "RJLauncher/1.0");

        azulList->clear();
        allJavaVersions = QJsonArray(); // Correct way to clear QJsonArray
        QNetworkReply *reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonArray packages = QJsonDocument::fromJson(reply->readAll()).array();
                QStringList seenVersions;

                for (const QJsonValue &v : packages) {
                    QJsonObject pkg = v.toObject();
                    
                    // Helper to convert version arrays [21, 30, 15] to "21.30.15"
                    auto parseVer = [](const QJsonValue &val) {
                        if (val.isArray()) {
                            QStringList parts;
                            for (const auto &p : val.toArray()) parts << p.toVariant().toString();
                            return parts.join('.');
                        }
                        return val.toVariant().toString();
                    };
                    
                    QString zuluVersion = parseVer(pkg["zulu_version"]);
                    QString javaVersion = parseVer(pkg["java_version"]);
                    
                    QString ver = QString("Zulu %1 (Java %2)").arg(zuluVersion, javaVersion);
                    if (seenVersions.contains(ver)) continue; // Prevent duplicates in UI
                    seenVersions << ver;

                    QListWidgetItem *item = new QListWidgetItem(ver, azulList);
                    item->setData(Qt::UserRole, pkg["download_url"].toString());
                    allJavaVersions.append(pkg); // Cache full package info
                }
            }
            reply->deleteLater();
        });
    }


    void onDownloadClicked() {
        QListWidget* currentList = currentSelectedList();

        if (!currentList || !currentList->currentItem()) {
            QMessageBox::warning(this, "No Selection", "Please select a Java version to download.");
            return;
        }

        QString selected = currentList->currentItem()->text();
        QString url;

        // Determine OS and Architecture
        QString os = QSysInfo::productType();
        QString arch = QSysInfo::currentCpuArchitecture();
        
        QString platform = (os == "windows") ? "windows" : "linux";
        QString architecture = (arch == "x86_64") ? "x64" : arch; 
        QString extension = (platform == "windows") ? ".zip" : ".tar.gz";

        if (currentList == azulList) {
            url = currentList->currentItem()->data(Qt::UserRole).toString(); // URL already stored in UserRole
        }
        // Find the selected version's full data from allJavaVersions
        QString downloadUrl = "";
        if (!url.isEmpty()) downloadUrl = url;

        if (downloadUrl.isEmpty()) {
            // Search in cached Adoptium versions for the download URL
            for (const QJsonValue &val : allJavaVersions) {
                QJsonObject releaseObj = val.toObject();
                if (releaseObj["openjdk_version"].toString() == selected) {
                    QJsonArray binaries = releaseObj["binaries"].toArray();
                    for (const QJsonValue &binVal : binaries) {
                        QJsonObject binary = binVal.toObject();
                        if (binary["os"].toString() == platform && binary["architecture"].toString() == architecture) {
                            downloadUrl = binary["package"].toObject()["link"].toString();
                            break;
                        }
                    }
                }
                if (!downloadUrl.isEmpty()) break;
            }
            // Fallback to a generic Adoptium latest API if not found in cache
            if (downloadUrl.isEmpty()) downloadUrl = QString("https://api.adoptium.net/v3/binary/latest/%1/ga/%2/%3/jdk/hotspot/normal/eclipse").arg(selected.section('-', -1), platform, architecture);
        }

        if (!downloadUrl.isEmpty()) {
            downloadAndExtract(downloadUrl, selected, extension);
        } else {
            LogLauncherEvent("Failed to find download link for " + selected);
            QMessageBox::critical(this, "Download Error", "Could not find a direct download link for the selected Java version and your OS/Architecture.");
        }
    }

    void downloadAndExtract(const QString &url, const QString &name, const QString &ext, QProgressDialog *externalProgress = nullptr) {
        LogLauncherEvent("Starting Java download from: " + url);
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        QDir().mkpath(dataRoot + "javas"); // Ensure the root javas folder exists
        
        // Improved sanitization to prevent "Failed to open file" errors
        QString safeName = name.simplified();
        safeName.replace(" ", "_");
        safeName.replace("(", "");
        safeName.replace(")", "");
        // Keep dots for versioning but replace for the zip/tar name if necessary
        QString fileSafeName = safeName;
        fileSafeName.replace(".", "_");
        
        QString fileName = fileSafeName + ext;
        QString filePath = dataRoot + "javas/" + fileName;
        QString extractDir = dataRoot + "javas/" + safeName;

        QProgressDialog *progress = externalProgress;
        if (!progress) {
            progress = new QProgressDialog("Downloading Java...", "Cancel", 0, 100, this);
            progress->setWindowModality(Qt::WindowModal);
            progress->show();
        }
        QDir().mkpath(extractDir); // Ensure specific version folder exists

        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));

        QEventLoop loop;
        connect(reply, &QNetworkReply::downloadProgress, [&](qint64 r, qint64 t) {
            if (t > 0) progress->setValue((r * 100) / t);
        });
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                progress->setLabelText("Extracting Java...");
                
                QProcess process;
                QStringList args;
#ifdef Q_OS_WIN
                // Windows extraction using PowerShell
                process.start("powershell", {"-Command", QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(filePath, extractDir)});
#else
                // Linux extraction using tar
                process.start("tar", {"-xzf", filePath, "-C", extractDir, "--strip-components=1"});
#endif
                process.waitForFinished();
                file.remove(); // Cleanup downloaded archive
                LogLauncherEvent("Java installed to: " + extractDir);
                if (!externalProgress) accept();
            } else {
                LogLauncherEvent("Failed to open file for writing Java: " + filePath);
            }
        } else {
            LogLauncherEvent("Java download failed: " + reply->errorString());
        }
        reply->deleteLater();
        if (!externalProgress) delete progress;
    }
};

void JavaDownloadDialog::AutoDownloadJava(const QString &javaName, QProgressDialog *externalProgress) {
    LogLauncherEvent("Headless Zulu Auto-Download for: " + javaName);
    QString os = (QSysInfo::kernelType() == "winnt") ? "windows" : "linux";
    QString archive = (os == "windows") ? "zip" : "tar.gz"; // Request specific archive type
    QString ver = javaName.contains("-") ? javaName.section('-', -1) : javaName;

    // Step 1: Fetch actual download URL from Zulu metadata API
    QString apiUrl = QString("https://api.azul.com/metadata/v1/zulu/packages/?os=%1&arch=x64&archive_type=%2&java_version=%3&java_package_type=jdk&latest=true")
                        .arg(os, archive, ver);

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(apiUrl)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString realDownloadUrl;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonArray packages = QJsonDocument::fromJson(reply->readAll()).array();
        if (!packages.isEmpty()) realDownloadUrl = packages.first().toObject()["download_url"].toString();
    }
    reply->deleteLater();

    if (!realDownloadUrl.isEmpty()) {
        JavaDownloadDialog *headless = new JavaDownloadDialog(nullptr, false);
        headless->downloadAndExtract(realDownloadUrl, "zulu-jdk-" + ver, "." + archive, externalProgress);
        delete headless;
    } else {
        LogLauncherEvent("Failed to find download URL for Java " + ver);
    }
}

void ShowJavaDownloadMenu(QWidget *parent) {
    JavaDownloadDialog dlg(parent);
    dlg.exec();
}

void AutoDownloadJavaHeadless(const QString &javaName, QProgressDialog *externalProgress) {
    JavaDownloadDialog::AutoDownloadJava(javaName, externalProgress);
}

#include "Java_download.moc"