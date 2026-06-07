// MadeChanges.cpp
// Complete, self-contained implementation for the MadeChanges dialog.
// Implements a robust single-download/extract/install flow and provides
// a safe implementation of on_get7zipButton_clicked to satisfy moc/linker.

#include "MadeChanges.h"
#include "ui_MadeChanges.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QTextCursor>
#include <QTemporaryDir>
#include <QDirIterator>
#include <QThread>
#include <QStandardPaths>
#include <QPointer>
#include <atomic>

// Fallback extractor using external 7z/7za if no library is provided.
#ifndef EXTRACTZIP_PROVIDED
static bool ExtractZipFile(const QString &zipPath, const QString &outDir)
{
    QFileInfo zf(zipPath);
    if (!zf.exists()) return false;
    QDir().mkpath(outDir);

    const QStringList candidates = { QStringLiteral("7z"), QStringLiteral("7za"), QStringLiteral("7z.exe"), QStringLiteral("7za.exe") };
    for (const QString &exe : candidates) {
        QProcess proc;
        QStringList args;
        args << QStringLiteral("x") << QStringLiteral("-y") << QStringLiteral("-o") + outDir << zipPath;
        proc.start(exe, args);
        if (!proc.waitForStarted(3000)) continue;
        if (!proc.waitForFinished(300000)) { proc.kill(); continue; }
        if (proc.exitCode() == 0) return true;
    }
    return false;
}
#endif

static QString humanReadableBytes(qint64 bytes)
{
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    double kb = bytes / 1024.0;
    if (kb < 1024) return QString::asprintf("%.2f KB", kb);
    double mb = kb / 1024.0;
    if (mb < 1024) return QString::asprintf("%.2f MB", mb);
    double gb = mb / 1024.0;
    return QString::asprintf("%.2f GB", gb);
}

static QString formatSeconds(qint64 secs)
{
    if (secs < 0) return QStringLiteral("--:--:--");
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    qint64 s = secs % 60;
    return QString::asprintf("%02lld:%02lld:%02lld", (long long)h, (long long)m, (long long)s);
}

static QUrl branchZipUrl()
{
    return QUrl(QStringLiteral("https://github.com/xrejenx/RJLauncher/archive/refs/heads/RJL.zip"));
}

static QString platformFolderName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("Win64");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Linux");
#elif defined(Q_OS_MAC)
    return QStringLiteral("Mac");
#else
    return QStringLiteral("Win64");
#endif
}

// Private implementation struct
class MadeChangesDialog::Impl {
public:
    Impl() = default;
    QNetworkAccessManager *netMgr = nullptr;
    QNetworkReply *currentReply = nullptr;
    QFile *currentFile = nullptr;

    QString toolsDir;
    QString downloadDir;
    QString zip7Dir;

    QElapsedTimer elapsed;
    std::atomic<qint64> receivedBytes{0};
    qint64 totalBytes = 0;
    double smoothedSpeed = 0.0;

    std::atomic<bool> cancelling{false};

    bool autoStart = false;
    QString cmdFileUrl;
};

MadeChangesDialog::MadeChangesDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::MadeChangesDialog),
    d(new Impl)
{
    ui->setupUi(this);

    QString exeDir = QCoreApplication::applicationDirPath();
    d->toolsDir = QDir(exeDir).filePath(QStringLiteral("Tools"));
    d->zip7Dir = QDir(d->toolsDir).filePath(QStringLiteral("7zip"));
    d->downloadDir = QDir(d->toolsDir).filePath(QStringLiteral("DownloadedZip"));
    QDir().mkpath(d->toolsDir);
    QDir().mkpath(d->zip7Dir);
    QDir().mkpath(d->downloadDir);

    d->netMgr = new QNetworkAccessManager(this);

    setWindowTitle(QStringLiteral("RJ Launcher Maintenance"));
    ui->titleLabel->setText(QStringLiteral("RJ Launcher Maintenance"));
    ui->statusLabel->setText(QStringLiteral("Select an action"));
    ui->progressBar->setVisible(false);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->logText->setReadOnly(true);
    ui->logText->setLineWrapMode(QTextEdit::WidgetWidth);
    ui->installButton->setEnabled(false);

    // Only Install and Cancel are wired here. The Get-7zip action is handled by the main launcher.
    connect(ui->installButton, &QPushButton::clicked, this, &MadeChangesDialog::on_installButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &MadeChangesDialog::on_cancelButton_clicked);

    // Command line options support
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringLiteral("auto-start"), QStringLiteral("Automatically start download/extract/install")));
    parser.addOption(QCommandLineOption(QStringLiteral("file-url"), QStringLiteral("URL of file to download"), QStringLiteral("url")));
    parser.process(*QCoreApplication::instance());

    if (parser.isSet(QStringLiteral("file-url"))) d->cmdFileUrl = parser.value(QStringLiteral("file-url"));
    if (parser.isSet(QStringLiteral("auto-start"))) d->autoStart = true;

    if (!d->cmdFileUrl.isEmpty()) {
        appendLogLine(QStringLiteral("[INFO] Received file-url: %1").arg(d->cmdFileUrl));
        setUpdatingMode(true);
        if (d->autoStart) {
            QTimer::singleShot(200, this, [this]() {
                startDownloadFromUrl(d->cmdFileUrl);
            });
        }
    }
}

MadeChangesDialog::~MadeChangesDialog()
{
    d->cancelling = true;
    if (d->currentReply) {
        d->currentReply->abort();
        d->currentReply->deleteLater();
        d->currentReply = nullptr;
    }
    if (d->currentFile) {
        if (d->currentFile->isOpen()) d->currentFile->close();
        delete d->currentFile;
        d->currentFile = nullptr;
    }
    delete ui;
    delete d;
}

void MadeChangesDialog::appendLogLine(const QString &line)
{
    ui->logText->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(line));
    QTextCursor c = ui->logText->textCursor();
    c.movePosition(QTextCursor::End);
    ui->logText->setTextCursor(c);
}

void MadeChangesDialog::setUpdatingMode(bool updating)
{
    if (updating) {
        setWindowTitle(QStringLiteral("RJ Launcher Updating"));
        ui->titleLabel->setText(QStringLiteral("RJ Launcher Updating"));
        ui->installButton->setEnabled(false);
        ui->cancelButton->setEnabled(true);
        ui->progressBar->setVisible(true);
    } else {
        setWindowTitle(QStringLiteral("RJ Launcher Maintenance"));
        ui->titleLabel->setText(QStringLiteral("RJ Launcher Maintenance"));
        ui->installButton->setEnabled(true);
        ui->cancelButton->setEnabled(true);
        ui->progressBar->setVisible(false);
        ui->progressBar->setValue(0);
        ui->speedLabel->setText(QString());
        ui->etaLabel->setText(QString());
        ui->bytesLabel->setText(QString());
    }
}

void MadeChangesDialog::setStatus(const QString &s)
{
    ui->statusLabel->setText(s);
}

// Implement the slot expected by moc to avoid linker errors.
// This dialog no longer performs the "Get 7zip" action itself; the main launcher Update tab handles that.
// Provide a helpful message so users know where to go.
void MadeChangesDialog::on_get7zipButton_clicked()
{
    QMessageBox::information(this,
                             QStringLiteral("Get 7zip"),
                             QStringLiteral("The launcher now downloads 7zip from the Update tab. Open the main launcher and use the Update -> Offline -> Get Lib button to fetch 7zip."));
}

// Robust single-download flow
void MadeChangesDialog::startDownloadFromUrl(const QString &urlString)
{
    if (urlString.isEmpty()) {
        appendLogLine(QStringLiteral("[ERROR] startDownloadFromUrl called with empty URL."));
        return;
    }

    QUrl url(urlString);
    if (!url.isValid()) {
        appendLogLine(QStringLiteral("[ERROR] Invalid URL passed to startDownloadFromUrl: %1").arg(urlString));
        QMessageBox::critical(this, QStringLiteral("Download"), QStringLiteral("Invalid URL: %1").arg(urlString));
        return;
    }

    QString fileName = QFileInfo(url.path()).fileName();
    if (fileName.isEmpty()) fileName = QStringLiteral("RJLauncher-RJL.zip");
    QString outPath = QDir(d->downloadDir).filePath(fileName);

    if (QFile::exists(outPath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            QStringLiteral("File exists"),
            QStringLiteral("A file named \"%1\" already exists in %2.\nDo you want to overwrite it?").arg(fileName, d->downloadDir),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply != QMessageBox::Yes) {
            appendLogLine(QStringLiteral("[INFO] User chose not to overwrite existing file: %1").arg(outPath));
            setUpdatingMode(false);
            return;
        }
        QFile::remove(outPath);
    }

    if (d->currentReply) {
        d->currentReply->abort();
        d->currentReply->deleteLater();
        d->currentReply = nullptr;
    }
    if (d->currentFile) {
        if (d->currentFile->isOpen()) d->currentFile->close();
        delete d->currentFile;
        d->currentFile = nullptr;
    }

    QFile *file = new QFile(outPath);
    if (!file->open(QIODevice::WriteOnly)) {
        appendLogLine(QStringLiteral("[ERROR] Cannot open file for writing: %1").arg(outPath));
        QMessageBox::critical(this, QStringLiteral("Download"), QStringLiteral("Cannot open file for writing: %1").arg(outPath));
        delete file;
        return;
    }

    appendLogLine(QStringLiteral("[INFO] Starting download: %1").arg(urlString));
    setUpdatingMode(true);
    setStatus(QStringLiteral("Downloading..."));
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->installButton->setEnabled(false);

    d->receivedBytes = 0;
    d->totalBytes = 0;
    d->smoothedSpeed = 0.0;
    d->cancelling = false;
    d->elapsed.restart();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "RJLauncher-MadeChanges");
    QNetworkReply *reply = d->netMgr->get(req);
    d->currentReply = reply;
    d->currentFile = file;

    QTimer *uiTimer = new QTimer(this);
    uiTimer->setInterval(500);
    connect(uiTimer, &QTimer::timeout, this, [this]() {
        qint64 received = d->receivedBytes.load();
        qint64 total = d->totalBytes;
        qint64 elapsedMs = d->elapsed.elapsed();
        if (elapsedMs <= 0) return;
        double speed = received / (elapsedMs / 1000.0);
        d->smoothedSpeed = (d->smoothedSpeed * 0.7) + (speed * 0.3);
        double speedMB = d->smoothedSpeed / (1024.0 * 1024.0);
        ui->speedLabel->setText(QString::asprintf("%.2f MB/s", speedMB));
        if (total > 0) {
            int pct = int((received * 100) / total);
            ui->progressBar->setValue(pct);
            qint64 remaining = total - received;
            qint64 eta = (d->smoothedSpeed > 0.1) ? qint64(remaining / d->smoothedSpeed) : -1;
            ui->etaLabel->setText(formatSeconds(eta));
            ui->bytesLabel->setText(QString("%1 / %2").arg(humanReadableBytes(received)).arg(humanReadableBytes(total)));
            setStatus(QStringLiteral("Downloading... %1%").arg(pct));
        } else {
            ui->etaLabel->setText(QStringLiteral("--:--:--"));
            ui->bytesLabel->setText(QString("%1").arg(humanReadableBytes(received)));
        }
    });
    uiTimer->start();

    QFile *filePtr = file;
    QNetworkReply *replyPtr = reply;
    QString outPathLocal = outPath;

    connect(reply, &QNetworkReply::readyRead, this, [this, filePtr, replyPtr]() {
        if (!filePtr || !replyPtr) return;
        QByteArray chunk = replyPtr->readAll();
        if (!chunk.isEmpty()) {
            qint64 written = filePtr->write(chunk);
            d->receivedBytes += written;
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        d->receivedBytes = received;
        d->totalBytes = total;
    });

    connect(reply, &QNetworkReply::finished, this, [this, outPathLocal, uiTimer, filePtr, replyPtr]() {
        if (uiTimer) {
            uiTimer->stop();
            uiTimer->deleteLater();
        }

        if (!replyPtr) {
            appendLogLine(QStringLiteral("[ERROR] Download reply missing at finish."));
            setUpdatingMode(false);
            return;
        }

        if (replyPtr->error() != QNetworkReply::NoError) {
            appendLogLine(QStringLiteral("[ERROR] Download failed: %1").arg(replyPtr->errorString()));
            setStatus(QStringLiteral("Download failed"));
            if (filePtr) {
                filePtr->close();
                delete filePtr;
            }
            replyPtr->deleteLater();
            d->currentReply = nullptr;
            d->currentFile = nullptr;
            ui->installButton->setEnabled(false);
            setUpdatingMode(false);
            QMessageBox::critical(this, QStringLiteral("Download Failed"), QStringLiteral("Download failed: %1").arg(replyPtr->errorString()));
            return;
        }

        if (filePtr && replyPtr) {
            QByteArray tail = replyPtr->readAll();
            if (!tail.isEmpty()) filePtr->write(tail);
            filePtr->flush();
            filePtr->close();
        }

        appendLogLine(QStringLiteral("[INFO] Download finished: %1").arg(outPathLocal));
        replyPtr->deleteLater();
        d->currentReply = nullptr;
        d->currentFile = nullptr;

        ui->installButton->setEnabled(true);
        setUpdatingMode(false);

        // Extraction
        QTemporaryDir tmp;
        if (!tmp.isValid()) {
            appendLogLine(QStringLiteral("[ERROR] Cannot create temporary directory for extraction."));
            QMessageBox::critical(this, QStringLiteral("Extraction Failed"), QStringLiteral("Cannot create temporary directory for extraction."));
            return;
        }
        QString extractTo = tmp.path();

        appendLogLine(QStringLiteral("[INFO] Extracting downloaded zip to temporary folder..."));
        setStatus(QStringLiteral("Extracting..."));
        ui->progressBar->setRange(0, 0);

        bool ok = ExtractZipFile(outPathLocal, extractTo);
        if (!ok) {
            appendLogLine(QStringLiteral("[WARN] ExtractZipFile failed; attempting 7z command-line."));
            QProcess p;
            QStringList args;
            args << QStringLiteral("x") << QStringLiteral("-y") << QStringLiteral("-o") + extractTo << outPathLocal;
            p.start(QStringLiteral("7z"), args);
            if (p.waitForStarted(3000) && p.waitForFinished(300000)) {
                ok = (p.exitCode() == 0);
            } else {
                ok = false;
            }
        }

        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(ok ? 100 : 0);

        if (!ok) {
            appendLogLine(QStringLiteral("[ERROR] Extraction failed."));
            setStatus(QStringLiteral("Extraction failed"));
            QMessageBox::critical(this, QStringLiteral("Extraction Failed"), QStringLiteral("Failed to extract the downloaded archive."));
            setUpdatingMode(false);
            return;
        }

        appendLogLine(QStringLiteral("[INFO] Extraction complete. Now attempting to install files."));

        QString platform = platformFolderName();
        QString foundFolder;
        QDirIterator it(extractTo, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString candidate = it.next();
            QString rel = QDir(extractTo).relativeFilePath(candidate);
            rel.replace('\\', '/');
            if (rel.endsWith(QStringLiteral("RJL/assets/7zip/%1").arg(platform))) {
                foundFolder = candidate;
                break;
            }
        }
        if (foundFolder.isEmpty()) {
            QDirIterator it2(extractTo, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it2.hasNext()) {
                QString candidate = it2.next();
                QFileInfo fi(candidate);
                if (fi.fileName().compare(QStringLiteral("RJL"), Qt::CaseInsensitive) == 0) {
                    QDir cand(candidate);
                    QString target = cand.filePath(QStringLiteral("assets/7zip/%1").arg(platform));
                    if (QDir(target).exists()) {
                        foundFolder = target;
                        break;
                    }
                }
            }
        }

        if (foundFolder.isEmpty()) {
            appendLogLine(QStringLiteral("[ERROR] Could not find RJL/assets/7zip/%1 inside extracted branch.").arg(platform));
            setUpdatingMode(false);
            QMessageBox::warning(this, QStringLiteral("Install"), QStringLiteral("Could not find platform folder inside the archive."));
            return;
        }

        appendLogLine(QStringLiteral("[INFO] Found platform folder: %1").arg(foundFolder));

        QDir targetDir(d->zip7Dir);
        QDir().mkpath(targetDir.path());

        QDir src(foundFolder);
        QDirIterator dit(src.path(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (dit.hasNext()) {
            QString sub = dit.next();
            QString rel = QDir(src.path()).relativeFilePath(sub);
            QString dest = targetDir.filePath(rel);
            QFileInfo sfi(sub);
            if (sfi.isDir()) {
                QDir().mkpath(dest);
            } else {
                QFile::remove(dest);
                if (!QFile::copy(sub, dest)) {
                    appendLogLine(QStringLiteral("[WARN] Failed to copy %1 to %2").arg(sub, dest));
                }
#ifndef Q_OS_WIN
                if (dest.endsWith(QLatin1String(".sh")) || dest.endsWith(QLatin1String(".run")) || dest.endsWith(QLatin1String(".bin"))) {
                    QFile::Permissions perms = QFile::permissions(dest);
                    perms |= QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther;
                    QFile::setPermissions(dest, perms);
                }
#endif
            }
        }

        appendLogLine(QStringLiteral("[INFO] Files copied to Tools/7zip. Installation complete."));
        setStatus(QStringLiteral("Installation complete"));
        ui->progressBar->setValue(100);
        setUpdatingMode(false);

        QString exe = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("RJML0444060626.exe"));
        if (QFile::exists(exe)) {
            appendLogLine(QStringLiteral("[INFO] Relaunching main launcher after install..."));
            QProcess::startDetached(exe, QStringList());
            QCoreApplication::quit();
        }
    });
}

void MadeChangesDialog::on_installButton_clicked()
{
    QDir dd(d->downloadDir);
    QStringList zips = dd.entryList(QStringList() << "*.zip" << "*.7z", QDir::Files, QDir::Time);
    if (zips.isEmpty()) {
        appendLogLine(QStringLiteral("[ERROR] No downloaded archive found to extract."));
        QMessageBox::warning(this, QStringLiteral("Install"), QStringLiteral("No downloaded archive found in %1").arg(d->downloadDir));
        return;
    }

    QString archive = dd.filePath(zips.first());
    appendLogLine(QStringLiteral("[INFO] Extracting %1").arg(archive));
    setStatus(QStringLiteral("Extracting..."));
    ui->progressBar->setRange(0, 0);

    bool ok = ExtractZipFile(archive, d->zip7Dir);
    if (!ok) {
        appendLogLine(QStringLiteral("[WARN] ExtractZipFile failed; attempting 7z command-line."));
        QProcess p;
        QStringList args;
        args << QStringLiteral("x") << QStringLiteral("-y") << QStringLiteral("-o") + d->zip7Dir << archive;
        p.start(QStringLiteral("7z"), args);
        if (p.waitForStarted(3000) && p.waitForFinished(300000)) {
            ok = (p.exitCode() == 0);
        } else {
            ok = false;
        }
    }

    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(ok ? 100 : 0);

    if (!ok) {
        appendLogLine(QStringLiteral("[ERROR] Extraction failed."));
        setStatus(QStringLiteral("Extraction failed"));
        QMessageBox::critical(this, QStringLiteral("Extraction Failed"), QStringLiteral("Failed to extract the downloaded archive."));
        setUpdatingMode(false);
        return;
    }

    appendLogLine(QStringLiteral("[INFO] Extraction complete. Files installed to Tools/7zip/"));
    setStatus(QStringLiteral("Installation complete"));
    ui->progressBar->setValue(100);
    setUpdatingMode(false);
    QMessageBox::information(this, QStringLiteral("Done"), QStringLiteral("Files installed to Tools/7zip/"));

    QString exe = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("RJML0444060626.exe"));
    if (QFile::exists(exe)) {
        appendLogLine(QStringLiteral("[INFO] Relaunching main launcher after install..."));
        QProcess::startDetached(exe, QStringList());
        QCoreApplication::quit();
    }
}

void MadeChangesDialog::on_cancelButton_clicked()
{
    appendLogLine(QStringLiteral("[ACTION] Cancel pressed."));
    setStatus(QStringLiteral("Cancelling..."));
    ui->cancelButton->setEnabled(false);

    d->cancelling = true;

    if (d->currentReply) {
        d->currentReply->abort();
        d->currentReply->deleteLater();
        d->currentReply = nullptr;
    }
    if (d->currentFile) {
        if (d->currentFile->isOpen()) d->currentFile->close();
        QString fname = d->currentFile->fileName();
        d->currentFile->remove();
        delete d->currentFile;
        d->currentFile = nullptr;
        appendLogLine(QStringLiteral("[INFO] Partial file removed: %1").arg(fname));
    }

    d->receivedBytes = 0;
    d->totalBytes = 0;
    d->smoothedSpeed = 0.0;
    d->cancelling = false;

    setUpdatingMode(false);
    appendLogLine(QStringLiteral("[INFO] Cancel complete."));
    ui->cancelButton->setEnabled(true);

    QString exe = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("RJML0444060626.exe"));
    if (QFile::exists(exe)) {
        appendLogLine(QStringLiteral("[INFO] Relaunching main launcher after cancel..."));
        QProcess::startDetached(exe, QStringList());
        QCoreApplication::quit();
    }
}

#include "MadeChanges.moc"
