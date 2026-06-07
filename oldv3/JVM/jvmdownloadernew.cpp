#include "jvmdownloadernew.h"
#include "downloadqueuedialog.h"
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, GetAppName
QString GetAppName();
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0); // From instance-javaRequirement.cpp

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QEventLoop>
#include <QStyleFactory>
#include <QFile>
#include <QDir>
#include <QSysInfo>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QApplication> // For QApplication::processEvents()

// External logger interface
void LogLauncherEvent(const QString &message);

// --- JavaVersionItemWidget Implementation ---
JavaVersionItemWidget::JavaVersionItemWidget(const QString &versionText, QWidget *parent)
    : QWidget(parent), listItem(nullptr), m_versionText(versionText) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 2, 10, 2);
    layout->setSpacing(10);

    // Fixed SIGSEGV: Checkbox must be initialized
    checkBox = new QCheckBox(this);
    layout->addWidget(checkBox);
    connect(checkBox, &QCheckBox::toggled, this, [this](bool checked) { emit itemChecked(listItem, checked); });

    versionLabel = new QLabel(versionText, this);
    versionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    versionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(versionLabel);

    progressBar = new QProgressBar(this);
    progressBar->setFixedHeight(24); // Taller progress bar
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setAlignment(Qt::AlignCenter); // Ensure text is centered
    progressBar->setVisible(false); // Hidden by default
    progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    progressBar->setMinimumWidth(250); // Prevent shrinking when long text is added
    layout->addWidget(progressBar);
}

void JavaVersionItemWidget::setProgress(int value, const QString &text) {
    progressBar->setValue(value);
    QString format = m_versionText + " : " + (text.isEmpty() ? "%p%" : text);
    progressBar->setFormat(format);
}

void JavaVersionItemWidget::showDownloadState(bool downloading) {
    progressBar->setVisible(downloading);
    versionLabel->setVisible(!downloading);
    checkBox->setVisible(!downloading); // Hide checkbox when downloading
    if (downloading) setProgress(0);
}

void JavaVersionItemWidget::setInstalled(bool installed) {
    if (installed) {
        versionLabel->setText(m_versionText + " (Installed)");
        checkBox->setEnabled(false);
    } else {
        versionLabel->setText(m_versionText);
        checkBox->setEnabled(true);
    }
}

// --- JVMDownloaderNew Implementation ---
JVMDownloaderNew::JVMDownloaderNew(QWidget *parent)
    : QDialog(parent), networkManager(new QNetworkAccessManager(this)) {
    setWindowTitle(GetAppName() + " - Download Java Runtime");
    setFixedSize(950, 550);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Filter Options
    QHBoxLayout *filterLayout = new QHBoxLayout();
    cbShowOld = new QCheckBox("Show Old Versions", this); // Maps to cbAllJava (disables &latest=true)
    cbShowAll = new QCheckBox("Show LTS Only", this); // Maps to cbLTS (enables &support_term=lts)
    cbShowAll->setChecked(true); // Default to LTS only
    filterLayout->addWidget(new QLabel("Filters:"));
    filterLayout->addWidget(cbShowAll);
    filterLayout->addWidget(cbShowOld);
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    mainLayout->addWidget(new QLabel("<b>Available Java Versions:</b>"));

    tabs = new QTabWidget(this);

    // Tab 1: Azul Zulu
    azulList = new QListWidget(this);
    // Native look: No custom stylesheet border
    tabs->addTab(azulList, "Java (Azul Zulu)");
    mainLayout->addWidget(tabs);
    connect(azulList, &QListWidget::itemClicked, this, &JVMDownloaderNew::onItemClicked);

    // Added native Download button at the bottom
    downloadSelectedButton = new QPushButton("Download Selected", this);
    downloadSelectedButton->setEnabled(false);
    downloadSelectedButton->setFixedHeight(40);
    mainLayout->addWidget(downloadSelectedButton);

    connect(downloadSelectedButton, &QPushButton::clicked, this, &JVMDownloaderNew::onDownloadSelectedClicked);

    connect(cbShowAll, &QCheckBox::toggled, this, [this](){ fetchAll(); }); 
    connect(cbShowOld, &QCheckBox::toggled, this, [this](){ fetchAll(); }); 
    
    fetchAll();
}

JVMDownloaderNew::~JVMDownloaderNew() {
    if (m_currentFetchReply) {
        // Use context-aware disconnect to be safe
        disconnect(m_currentFetchReply, nullptr, this, nullptr);
        m_currentFetchReply->abort();
        m_currentFetchReply->deleteLater();
        m_currentFetchReply = nullptr;
    }
}

void JVMDownloaderNew::fetchAll() {
    fetchAzulVersions();
}

QListWidget* JVMDownloaderNew::currentSelectedList() {
    if (tabs->currentIndex() == 0) return azulList;
    return nullptr;
}

void JVMDownloaderNew::updateDownloadButtonState() {
    bool anyChecked = false;
    for (int i = 0; i < azulList->count(); ++i) {
        JavaVersionItemWidget* widget = qobject_cast<JavaVersionItemWidget*>(azulList->itemWidget(azulList->item(i)));
        if (widget && widget->checkBox->isChecked()) {
            anyChecked = true;
            break;
        }
    }
    downloadSelectedButton->setEnabled(anyChecked);
}

void JVMDownloaderNew::fetchAzulVersions() {
    QString os = (QSysInfo::kernelType() == "winnt") ? "windows" : "linux";
    QString arch = (QSysInfo::currentCpuArchitecture() == "x86_64") ? "x64" : QSysInfo::currentCpuArchitecture();

    QString ltsFilter = cbShowAll->isChecked() ? "&support_term=lts" : "";
    QString latestFilter = cbShowOld->isChecked() ? "" : "&latest=true"; // "Show Old" means don't filter by latest
    QString gaFilter = "&release_status=ga";

    QUrl url(QString("https://api.azul.com/metadata/v1/zulu/packages/?os=%1&arch=%2&java_package_type=jdk%3%4%5")
             .arg(os, arch, ltsFilter, latestFilter, gaFilter));
             
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "RJLauncher/1.0");

    // Abort existing fetch to prevent concurrent list modification
    if (m_currentFetchReply) {
        m_currentFetchReply->abort();
        m_currentFetchReply->deleteLater();
    }

    azulList->clear();
    allAzulVersions = QJsonArray();
    
    QNetworkReply *reply = networkManager->get(request);
    m_currentFetchReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // Safety: If this isn't the reply we're looking for anymore, ignore it
        if (!reply || reply != m_currentFetchReply) return;
        m_currentFetchReply = nullptr;
        
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray packages = QJsonDocument::fromJson(reply->readAll()).array();
            QStringList seenVersions;

            for (const QJsonValue &v : packages) {
                QJsonObject pkg = v.toObject();
                
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
                if (seenVersions.contains(ver)) continue;
                seenVersions << ver;

                QListWidgetItem *item = new QListWidgetItem(azulList);
                item->setSizeHint(QSize(azulList->width(), 40)); // Fixed height for custom widget
                
                JavaVersionItemWidget *itemWidget = new JavaVersionItemWidget(ver, azulList);
                itemWidget->listItem = item; // Set back-pointer
                azulList->setItemWidget(item, itemWidget);
                connect(itemWidget, &JavaVersionItemWidget::itemChecked, this, &JVMDownloaderNew::updateDownloadButtonState);
                
                // Store download URL and full package info in item's data
                item->setData(Qt::UserRole, pkg["download_url"].toString());
                item->setData(Qt::UserRole + 1, QVariant::fromValue(pkg)); // Store full JSON object

                // Check if already installed
                QString javasRoot = MinecraftLauncher::getRJLDataPath() + "javas";
                QDir jDir(javasRoot);
                bool isInstalled = false;
                for (const QString &sub : jDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    if (sub.contains(javaVersion) || sub.contains(zuluVersion)) {
                        isInstalled = true;
                        break;
                    }
                }

                if (isInstalled) {
                    itemWidget->checkBox->setChecked(false); // Ensure it's unchecked
                    itemWidget->setInstalled(true);
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled); // Greys out and disables
                }

                allAzulVersions.append(pkg);
            }
        } else {
            LogLauncherEvent("Failed to fetch Azul versions: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void JVMDownloaderNew::onDownloadSelectedClicked() {
    QList<QListWidgetItem*> checkedItems;
    for (int i = 0; i < azulList->count(); ++i) {
        QListWidgetItem* item = azulList->item(i);
        JavaVersionItemWidget* widget = qobject_cast<JavaVersionItemWidget*>(azulList->itemWidget(item));
        if (widget && widget->checkBox->isChecked() && (item->flags() & Qt::ItemIsEnabled)) {
            checkedItems.append(item);
        }
    }

    if (checkedItems.isEmpty()) return;

    // Use the queue dialog to handle multiple downloads
    DownloadQueueDialog *queue = new DownloadQueueDialog(checkedItems, this);
    queue->exec();
    queue->deleteLater();
}

void JVMDownloaderNew::onItemClicked(QListWidgetItem* item) {
    JavaVersionItemWidget *itemWidget = qobject_cast<JavaVersionItemWidget*>(azulList->itemWidget(item));
    if (itemWidget && itemWidget->checkBox->isEnabled()) {
        itemWidget->checkBox->setChecked(!itemWidget->checkBox->isChecked());
    }
}

void ShowJVMDownloaderNew(QWidget *parent) {
    JVMDownloaderNew dlg(parent);
    dlg.exec();
}
