// updater.cpp
// Full rewrite of updater implementation. Integrates release listing and direct raw-file download
// for 7za binaries (Windows/Linux) and retains UI wiring for the LauncherUpdate tab.
// Assumes Updater class declared as: Updater(MainWindow *parentWindow) and members used below.

#include "updater.h"
#include "mainwindow.h"
#include "version.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QProcess>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDebug>

// -----------------------------
// Constructor / destructor
// -----------------------------
Updater::Updater(MainWindow *parentWindow)
    : QObject(parentWindow),
    m_mainWindow(parentWindow),
    m_netMgr(new QNetworkAccessManager(this)),
    m_currentDownloadReply(nullptr),
    m_currentDownloadFile(nullptr)
{
    // Default download folder inside user's Downloads/RJLauncherDownloads
    QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloads.isEmpty()) downloads = QDir::homePath();
    m_downloadDir = QDir(downloads).filePath("RJLauncherDownloads");
    QDir().mkpath(m_downloadDir);

    // Connect only the releases fetch handler to the manager's finished signal
    connect(m_netMgr, &QNetworkAccessManager::finished, this, &Updater::onReleasesFetched);

    // Initialize UI state if available
    if (m_mainWindow && m_mainWindow->currentBuildLabel()) {
        m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
    }
}

Updater::~Updater()
{
    if (m_currentDownloadReply) {
        m_currentDownloadReply->abort();
        m_currentDownloadReply->deleteLater();
        m_currentDownloadReply = nullptr;
    }
    if (m_currentDownloadFile) {
        if (m_currentDownloadFile->isOpen()) m_currentDownloadFile->close();
        delete m_currentDownloadFile;
        m_currentDownloadFile = nullptr;
    }
}

// -----------------------------
// Helpers
// -----------------------------
QString Updater::makeUserAgent() const
{
    return QString("RJLauncher-Updater/%1").arg(AppVersion::appBuildNumber());
}

static QString simpleMarkdownToHtml(const QString &md)
{
    QString html = md.toHtmlEscaped();
    html.replace(QRegularExpression("^###\\s+(.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^##\\s+(.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^#\\s+(.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");

    QStringList lines = html.split('\n');
    QString out;
    bool inList = false;
    for (const QString &ln : lines) {
        QString t = ln.trimmed();
        if (t.startsWith("- ") || t.startsWith("* ")) {
            if (!inList) { out += "<ul>"; inList = true; }
            QString item = t.mid(2).trimmed();
            out += "<li>" + item + "</li>";
        } else {
            if (inList) { out += "</ul>"; inList = false; }
            if (!t.isEmpty()) out += "<p>" + t + "</p>";
        }
    }
    if (inList) out += "</ul>";
    return out;
}

// -----------------------------
// Public API
// -----------------------------
void Updater::initUpdater()
{
    // Reset UI lists
    if (m_mainWindow) {
        if (m_mainWindow->remoteVersionsList()) m_mainWindow->remoteVersionsList()->clear();
        if (m_mainWindow->filesToDownloadList()) m_mainWindow->filesToDownloadList()->clear();
        if (m_mainWindow->releaseDescription()) m_mainWindow->releaseDescription()->clear();
        if (m_mainWindow->downloadButton()) m_mainWindow->downloadButton()->setEnabled(false);
    }

    // Fetch releases (non-authenticated; consider token if rate-limited)
    QUrl apiUrl("https://api.github.com/repos/xrejenx/RJLauncher/releases");
    QNetworkRequest req(apiUrl);
    req.setRawHeader("User-Agent", makeUserAgent().toUtf8());
    req.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_netMgr->get(req);
}

// -----------------------------
// Releases handling
// -----------------------------
void Updater::onReleasesFetched(QNetworkReply *reply)
{
    // This slot is connected to QNetworkAccessManager::finished; ensure this reply is for releases
    if (!reply) return;

    // If this reply was used for an asset download, ignore here (asset downloads use separate reply objects
    // created by other QNetworkAccessManager calls and handled by dedicated slots).
    // We identify releases fetch by URL path containing "/releases"
    QUrl reqUrl = reply->request().url();
    if (!reqUrl.path().contains("/releases")) {
        // Not a releases response; ignore here
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        if (m_mainWindow && m_mainWindow->currentBuildLabel())
            m_mainWindow->currentBuildLabel()->setText(QString("Updater error: %1").arg(err));
        reply->deleteLater();
        // restore build label after short delay or immediately
        if (m_mainWindow && m_mainWindow->currentBuildLabel()) {
            m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
        }
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    parseReleases(data);
    populateRemoteList();

    if (m_mainWindow && m_mainWindow->currentBuildLabel()) {
        m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
    }

    // Wire UI connections after population
    setupConnections();
}

void Updater::parseReleases(const QByteArray &jsonData)
{
    m_releases.clear();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Updater: JSON parse error:" << err.errorString();
        return;
    }

    if (!doc.isArray()) return;
    QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        RemoteRelease r;
        r.name = o.value("name").toString(o.value("tag_name").toString("Unnamed"));
        r.tag  = o.value("tag_name").toString();
        r.body = o.value("body").toString();

        QJsonArray assets = o.value("assets").toArray();
        for (const QJsonValue &av : assets) {
            if (!av.isObject()) continue;
            QJsonObject ao = av.toObject();
            RemoteAsset a;
            a.name = ao.value("name").toString();
            a.url  = ao.value("browser_download_url").toString();
            a.size = ao.value("size").toInt(0);
            if (!a.name.isEmpty() && !a.url.isEmpty()) r.assets.append(a);
        }
        m_releases.append(r);
    }
}

void Updater::populateRemoteList()
{
    if (!m_mainWindow) return;
    QListWidget *list = m_mainWindow->remoteVersionsList();
    if (!list) return;
    list->clear();

    for (int i = 0; i < m_releases.size(); ++i) {
        const RemoteRelease &r = m_releases.at(i);

        QString label;
        if (!r.name.isEmpty()) {
            label = r.name;
        } else if (!r.tag.isEmpty()) {
            label = QString("RJ Launcher %1").arg(r.tag);
        } else {
            label = QString("RJ Launcher (release %1)").arg(i+1);
        }

        QListWidgetItem *it = new QListWidgetItem(label, list);
        it->setData(Qt::UserRole, i);
        it->setData(Qt::UserRole + 1, r.body);

        // store assets as compact JSON in item data
        QJsonArray aset;
        for (const RemoteAsset &a : r.assets) {
            QJsonObject ao;
            ao["name"] = a.name;
            ao["url"]  = a.url;
            ao["size"] = (double)a.size;
            aset.append(ao);
        }
        it->setData(Qt::UserRole + 2, QJsonDocument(aset).toJson(QJsonDocument::Compact));
    }

    if (list->count() > 0) {
        list->setCurrentRow(0);
        QListWidgetItem *first = list->item(0);
        if (first) {
            // trigger click behavior manually
            emit list->itemClicked(first);
        }
    }
}

// -----------------------------
// UI wiring
// -----------------------------
void Updater::setupConnections()
{
    if (!m_mainWindow) return;

    // When a release is selected, populate release notes and assets list
    if (m_mainWindow->remoteVersionsList()) {
        connect(m_mainWindow->remoteVersionsList(), &QListWidget::itemClicked, m_mainWindow, [this](QListWidgetItem *item){
            if (!item || !m_mainWindow) return;
            QString body = item->data(Qt::UserRole + 1).toString();
            if (m_mainWindow->releaseDescription())
                m_mainWindow->releaseDescription()->setHtml(simpleMarkdownToHtml(body));

            if (m_mainWindow->filesToDownloadList()) m_mainWindow->filesToDownloadList()->clear();
            QByteArray asetJson = item->data(Qt::UserRole + 2).toByteArray();
            QJsonDocument doc = QJsonDocument::fromJson(asetJson);
            if (doc.isArray() && m_mainWindow->filesToDownloadList()) {
                for (const QJsonValue &v : doc.array()) {
                    QJsonObject ao = v.toObject();
                    QString name = ao.value("name").toString();
                    QListWidgetItem *ai = new QListWidgetItem(name, m_mainWindow->filesToDownloadList());
                    ai->setData(Qt::UserRole, ao.value("url").toString());
                    ai->setFlags(ai->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    ai->setCheckState(Qt::Unchecked);
                }
                if (m_mainWindow->filesToDownloadList()->count() > 0) {
                    QListWidgetItem *firstAsset = m_mainWindow->filesToDownloadList()->item(0);
                    if (firstAsset) {
                        bool blocked = m_mainWindow->filesToDownloadList()->blockSignals(true);
                        firstAsset->setCheckState(Qt::Checked);
                        m_mainWindow->filesToDownloadList()->blockSignals(blocked);
                    }
                }
            }
            if (m_mainWindow->downloadButton())
                m_mainWindow->downloadButton()->setEnabled(m_mainWindow->filesToDownloadList() && m_mainWindow->filesToDownloadList()->count() > 0);
        });
    }

    // Ensure only one asset can be checked at a time
    if (m_mainWindow->filesToDownloadList()) {
        connect(m_mainWindow->filesToDownloadList(), &QListWidget::itemChanged, m_mainWindow, [this](QListWidgetItem *changed){
            if (!changed || !m_mainWindow) return;
            if (changed->checkState() == Qt::Checked) {
                QListWidget *files = m_mainWindow->filesToDownloadList();
                if (!files) return;
                bool blocked = files->blockSignals(true);
                for (int i = 0; i < files->count(); ++i) {
                    QListWidgetItem *it = files->item(i);
                    if (!it || it == changed) continue;
                    if (it->checkState() == Qt::Checked) it->setCheckState(Qt::Unchecked);
                }
                files->blockSignals(blocked);
            }
            if (m_mainWindow->downloadButton()) {
                bool hasItems = m_mainWindow->filesToDownloadList() && m_mainWindow->filesToDownloadList()->count() > 0;
                m_mainWindow->downloadButton()->setEnabled(hasItems);
            }
        });
    }

    // Download button: either use selected asset or fallback to first asset
    if (m_mainWindow->downloadButton()) {
        connect(m_mainWindow->downloadButton(), &QPushButton::clicked, m_mainWindow, [this](){
            if (!m_mainWindow) return;
            QListWidgetItem *selectedItem = nullptr;
            if (m_mainWindow->filesToDownloadList()) {
                for (int i = 0; i < m_mainWindow->filesToDownloadList()->count(); ++i) {
                    QListWidgetItem *it = m_mainWindow->filesToDownloadList()->item(i);
                    if (!it) continue;
                    if (it->checkState() == Qt::Checked) { selectedItem = it; break; }
                }
                if (!selectedItem && m_mainWindow->filesToDownloadList()->count() > 0)
                    selectedItem = m_mainWindow->filesToDownloadList()->item(0);
            }

            if (!selectedItem) {
                QMessageBox::information(m_mainWindow, "Download", "No files selected to download.");
                return;
            }

            QString url  = selectedItem->data(Qt::UserRole).toString();
            QString name = selectedItem->text();
            RemoteAsset a; a.name = name; a.url = url;

            // If the selected asset is a direct raw URL to a binary in the repo, download internally.
            // Otherwise, prefer launching the external MadeChanges tool if present.
            // Heuristic: if URL contains "raw.githubusercontent.com" or ends with known binary names, download internally.
            if (url.contains("raw.githubusercontent.com") || url.endsWith(".exe") || url.endsWith(".zip") || url.endsWith(".7z")) {
                startInternalAssetDownload(a);
            } else {
                // fallback: try external tool
                startAssetDownload(a);
            }
        });
    }

    // Double-click toggles check state
    if (m_mainWindow->filesToDownloadList()) {
        connect(m_mainWindow->filesToDownloadList(), &QListWidget::itemDoubleClicked, m_mainWindow, [](QListWidgetItem *it){
            if (!it) return;
            it->setCheckState(it->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
        });
    }
}

// -----------------------------
// External tool flow (existing behavior)
// -----------------------------
void Updater::startAssetDownload(const RemoteAsset &asset)
{
    if (!m_mainWindow) return;

    // Build path to MadeChanges executable next to the launcher
    QString exeDir = QCoreApplication::applicationDirPath();
    QString toolPath = QDir(exeDir).filePath("Tools/MadeChanges.exe");

    if (!QFile::exists(toolPath)) {
        QMessageBox::critical(m_mainWindow, "Updater", QString("Updater tool not found: %1").arg(toolPath));
        return;
    }

    // Prepare arguments: pass the download URL and request auto-start
    QStringList args;
    args << "--file-url" << asset.url << "--auto-start";

    // Update main UI to indicate update is happening
    if (m_mainWindow->currentBuildLabel()) {
        m_mainWindow->currentBuildLabel()->setText(QStringLiteral("RJ Launcher Updating"));
    }
    if (m_mainWindow->releaseDescription()) {
        m_mainWindow->releaseDescription()->setPlainText(QStringLiteral("Update is being handled by the external updater tool. See the tool's log for progress."));
    }

    // Start the external tool detached so it can run independently
    bool started = QProcess::startDetached(toolPath, args);
    if (!started) {
        QMessageBox::critical(m_mainWindow, "Updater", "Failed to launch updater tool.");
        if (m_mainWindow->currentBuildLabel())
            m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
        return;
    }

    // Optionally quit the main launcher so the tool can replace files if needed.
    // QCoreApplication::quit();
}

// -----------------------------
// Internal direct-download flow (rewritten to download raw files directly)
// -----------------------------
void Updater::startInternalAssetDownload(const RemoteAsset &asset)
{
    // If a download is already in progress, reject
    if (m_currentDownloadReply) {
        QMessageBox::information(m_mainWindow, "Download", "A download is already in progress.");
        return;
    }

    // If the asset URL is empty, return
    if (asset.url.isEmpty()) {
        QMessageBox::warning(m_mainWindow, "Download", "Asset URL is empty.");
        return;
    }

    // Prepare target folder: Tools/7zip (or keep original file name for other assets)
    QString exeDir = QCoreApplication::applicationDirPath();
    QString toolsDir = QDir(exeDir).filePath("Tools/7zip");
    QDir().mkpath(toolsDir);

    // Determine destination filename
    QString destName = QFileInfo(QUrl(asset.url).path()).fileName();
    if (destName.isEmpty()) destName = asset.name;
    if (destName.isEmpty()) destName = QStringLiteral("downloaded_asset.bin");

    QString outPath = QDir(toolsDir).filePath(destName);

    // Create file for writing
    m_currentDownloadFile = new QFile(outPath);
    if (!m_currentDownloadFile->open(QIODevice::WriteOnly)) {
        QMessageBox::critical(m_mainWindow, "Download", QString("Cannot open file for writing: %1").arg(outPath));
        delete m_currentDownloadFile;
        m_currentDownloadFile = nullptr;
        return;
    }

    // Start network request
    QNetworkRequest req(QUrl(asset.url));
    req.setRawHeader("User-Agent", makeUserAgent().toUtf8());
    m_currentDownloadReply = m_netMgr->get(req);

    // Connect progress and finished handlers specifically for this reply
    connect(m_currentDownloadReply, &QNetworkReply::downloadProgress, this, &Updater::onAssetDownloadProgress);
    connect(m_currentDownloadReply, &QNetworkReply::readyRead, this, &Updater::onAssetDownloadReadyRead);
    connect(m_currentDownloadReply, &QNetworkReply::finished, this, &Updater::onAssetDownloadedFinished);

    // Update UI
    if (m_mainWindow && m_mainWindow->currentBuildLabel()) {
        m_mainWindow->currentBuildLabel()->setText(QString("Downloading %1").arg(destName));
    }
}

void Updater::onAssetDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!m_mainWindow) return;
    if (m_mainWindow->currentBuildLabel()) {
        if (bytesTotal > 0) {
            int pct = int((bytesReceived * 100) / bytesTotal);
            m_mainWindow->currentBuildLabel()->setText(QString("Downloading... %1%").arg(pct));
        } else {
            m_mainWindow->currentBuildLabel()->setText(QString("Downloading... %1 bytes").arg(bytesReceived));
        }
    }
}

void Updater::onAssetDownloadReadyRead()
{
    if (!m_currentDownloadReply || !m_currentDownloadFile) return;
    QByteArray chunk = m_currentDownloadReply->readAll();
    if (!chunk.isEmpty()) m_currentDownloadFile->write(chunk);
}

void Updater::onAssetDownloadedFinished()
{
    if (!m_mainWindow) return;
    if (!m_currentDownloadReply) return;

    // Check for errors
    if (m_currentDownloadReply->error() != QNetworkReply::NoError) {
        QString err = m_currentDownloadReply->errorString();
        if (m_mainWindow->currentBuildLabel()) m_mainWindow->currentBuildLabel()->setText("Updater error: " + err);
        if (m_currentDownloadFile) {
            m_currentDownloadFile->close();
            m_currentDownloadFile->remove();
            delete m_currentDownloadFile;
            m_currentDownloadFile = nullptr;
        }
        m_currentDownloadReply->deleteLater();
        m_currentDownloadReply = nullptr;

        if (m_mainWindow->currentBuildLabel())
            m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
        return;
    }

    // Finalize file
    if (m_currentDownloadFile && m_currentDownloadReply) {
        QByteArray tail = m_currentDownloadReply->readAll();
        if (!tail.isEmpty()) m_currentDownloadFile->write(tail);
        m_currentDownloadFile->flush();
        m_currentDownloadFile->close();
    }

    QString savedPath;
    if (m_currentDownloadFile) {
        savedPath = m_currentDownloadFile->fileName();
        delete m_currentDownloadFile;
        m_currentDownloadFile = nullptr;
    }

    m_currentDownloadReply->deleteLater();
    m_currentDownloadReply = nullptr;

    // Set executable bit on Linux if appropriate
#ifndef Q_OS_WIN
    if (!savedPath.isEmpty()) {
        QFileInfo fi(savedPath);
        if (fi.exists()) {
            if (savedPath.endsWith(QLatin1String(".sh")) || savedPath.endsWith(QLatin1String(".run")) ||
                savedPath.endsWith(QLatin1String(".bin")) || fi.suffix().isEmpty()) {
                QFile::Permissions perms = QFile::permissions(savedPath);
                perms |= QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther;
                QFile::setPermissions(savedPath, perms);
            }
        }
    }
#endif

    if (m_mainWindow->currentBuildLabel())
        m_mainWindow->currentBuildLabel()->setText(QString("Downloaded to: %1").arg(savedPath));
    QMessageBox::information(m_mainWindow, "Download Complete", QString("Saved to: %1").arg(savedPath));

    // If the downloaded file is an executable for this platform, attempt to launch it
    if (!savedPath.isEmpty()) {
#ifdef Q_OS_WIN
        if (savedPath.endsWith(".exe", Qt::CaseInsensitive)) {
            QProcess::startDetached(savedPath, QStringList());
        }
#else
        QFileInfo fi(savedPath);
        if (fi.isExecutable() || savedPath.endsWith(".sh") || savedPath.endsWith(".run")) {
            QProcess::startDetached(savedPath, QStringList());
        }
#endif
    }

    if (m_mainWindow->currentBuildLabel())
        m_mainWindow->currentBuildLabel()->setText(QString("Build Number : %1").arg(AppVersion::appBuildNumber()));
}

// -----------------------------
// Convenience: direct 7za download helper (call from UI or on first launch)
// -----------------------------
void Updater::downloadSevenZipDirectly()
{
    // Choose raw URLs provided by user
#ifdef Q_OS_WIN
    QString libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Windows7zip/7za.exe";
    QString destinationFileName = "7za.exe";
#else
    QString libUrl = "https://raw.githubusercontent.com/xrejenx/RJ-MinecraftLauncher/RJL/assets/7zip/Linux7zip/7za";
    QString destinationFileName = "7za";
#endif

    // Build RemoteAsset and use internal download flow
    RemoteAsset a;
    a.name = destinationFileName;
    a.url = libUrl;
    startInternalAssetDownload(a);
}

// -----------------------------
// End of file
// -----------------------------
