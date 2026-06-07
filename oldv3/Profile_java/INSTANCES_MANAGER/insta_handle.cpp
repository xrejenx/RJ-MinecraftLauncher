#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCompleter>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include "Core.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

/**
 * insta_handle.cpp - Wizard for creating new Minecraft or Modpack instances
 */

// Forward declaration for the vanilla version selector
void LogLauncherEvent(const QString &message);
QString ShowVanillaVersionSelector(QWidget *parent);

// From vanilla_mc_handle.cpp & vanilla_mc_downloader.cpp
QVariantMap FetchVanillaMetadata(const QString &versionId); 
QVariantMap FetchModloaderMetadata(const QString &loader, const QString &mcVersion, const QString &loaderVersion);
void DownloadVanillaInstance(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId);
void CreateLaunchArgs(const QString &targetDirPath, const QString &versionId);
int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0);
QString GetLwglVersionForMc(const QString &mcVersion);

class InstanceWizard : public QDialog {
    Q_OBJECT
public:
    InstanceWizard(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("New Instance Wizard");
        setFixedSize(500, 500);
        auto *mainLayout = new QVBoxLayout(this);
        tabs = new QTabWidget(this);
        tabs->tabBar()->hide(); // Hide tabs to force button navigation
        
        selectedLoader = "Vanilla";
        networkManager = new QNetworkAccessManager(this);
        fetchMcVersions();

        // Tab 1: Minecraft or Modpack Selection
        auto *t1 = new QWidget();
        auto *l1 = new QVBoxLayout(t1);
        l1->setAlignment(Qt::AlignTop);

        QGroupBox *typeGroupBox = new QGroupBox("Choose Instance Type", t1);
        QVBoxLayout *typeLayout = new QVBoxLayout(typeGroupBox);
        chkMinecraft = new QCheckBox("Minecraft (Vanilla / Modded)", t1);
        chkModpack = new QCheckBox("Modpack (Modrinth / CurseForge)", t1);

        typeLayout->addWidget(chkMinecraft);
        typeLayout->addWidget(chkModpack);
        l1->addWidget(typeGroupBox);
        l1->addStretch();
        tabs->addTab(t1, "1. Type");

        // Tab 2: Modloader or Downloader
        auto *t2 = new QWidget();
        stackT2 = new QStackedWidget(t2);
        
        QWidget *viewMc = new QWidget();
        QVBoxLayout *lMc = new QVBoxLayout(viewMc);
        lMc->setSpacing(0); // Reduce spacing between widgets
        lMc->setContentsMargins(0, 0, 0, 0); // Remove outer margins
        lMc->setAlignment(Qt::AlignTop); // Ensure content sticks to the top

        QVBoxLayout *comboLayout = new QVBoxLayout();
        comboLayout->setSpacing(5);

        versionCombo = new QComboBox(this);
        versionCombo->setPlaceholderText("Select Minecraft Version...");
        versionCombo->setFixedHeight(35);
        versionCombo->setEditable(true);
        versionCombo->setInsertPolicy(QComboBox::NoInsert);

        loaderVersionCombo = new QComboBox(this);
        loaderVersionCombo->setPlaceholderText("Select Loader Version...");
        loaderVersionCombo->setFixedHeight(35);
        loaderVersionCombo->setEditable(true); // Allow manual version input
        loaderVersionCombo->setVisible(false);

        if (versionCombo->completer()) {
            versionCombo->completer()->setFilterMode(Qt::MatchContains);
            versionCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
        }

        comboLayout->addWidget(versionCombo);
        comboLayout->addWidget(loaderVersionCombo);
        comboLayout->addStretch();

        // Version Filters
        QGroupBox *filterGroupBox = new QGroupBox("Version Filters", this);
        filterGroupBox->setFlat(true); // Makes the box more compact
        QHBoxLayout *filterLayout = new QHBoxLayout(filterGroupBox);
        filterLayout->setContentsMargins(5, 5, 5, 5); // Keep some padding inside the group box
        filterLayout->setSpacing(5); // Reduce spacing between checkboxes
        chkAlpha = new QCheckBox("Show Alpha", this);
        chkBeta = new QCheckBox("Show Beta", this);
        chkSnapshot = new QCheckBox("Snapshots", this);
        filterLayout->addWidget(chkAlpha);
        filterLayout->addWidget(chkBeta);
        filterLayout->addWidget(chkSnapshot);
        filterLayout->addStretch(); // Push checkboxes to the left
        filterGroupBox->setLayout(filterLayout); // Set the layout for the group box
        filterGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // Ensure it doesn't expand unnecessarily
        lMc->addWidget(filterGroupBox);
        
        auto *loaderList = new QListWidget(this);
        loaderList->setFixedHeight(150);
        loaderList->addItems({"Vanilla", "Forge", "Fabric", "Quilt", "NeoForge"});
        vanillaListItem = loaderList->findItems("Vanilla", Qt::MatchExactly).first();
        
        connect(loaderList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
            selectedLoader = item->text();
            bool isVanilla = (selectedLoader == "Vanilla");
            
            loaderVersionCombo->setVisible(!isVanilla);
            loaderVersionCombo->clear();
            
            if (versionCombo->count() == 0) populateVersionCombo();
            
            updateInstanceName();
            updateNavButtons();
            
            if (!isVanilla && !selectedVersion.isEmpty()) {
                fetchLoaderVersions(selectedLoader, selectedVersion);
            }
        });

        QHBoxLayout *selectionRow = new QHBoxLayout();
        selectionRow->setSpacing(5);
        selectionRow->addWidget(loaderList, 2);
        selectionRow->addLayout(comboLayout, 3);
        lMc->addLayout(selectionRow); 

        // Instance Setting Section
        QGroupBox *settingsGroupBox = new QGroupBox("Instance Setting", this);
        QFormLayout *settingsLayout = new QFormLayout();
        settingsLayout->setContentsMargins(5, 5, 5, 5); // Standard form layout padding
        settingsLayout->setSpacing(5); // Standard form layout spacing

        nameEdit = new QLineEdit(this);
        nameEdit->setPlaceholderText("Enter instance name...");

        minRamEdit = new QLineEdit(this);
        minRamEdit->setPlaceholderText("Default: 512M");
        maxRamEdit = new QLineEdit(this);
        maxRamEdit->setPlaceholderText("Default: 2G");

        settingsLayout->addRow("Instance Name:", nameEdit);
        settingsLayout->addRow("Minimum RAM:", minRamEdit);
        settingsLayout->addRow("Maximum RAM:", maxRamEdit);

        moreOptionsToggle = new QCheckBox("Show More Options", this);
        settingsLayout->addRow(moreOptionsToggle);

        moreOptionsWidget = new QWidget(this);
        QFormLayout *moreLayout = new QFormLayout();
        moreLayout->setContentsMargins(0, 0, 0, 0);

        permGenEdit = new QLineEdit(this);
        permGenEdit->setPlaceholderText("Default: 128M");

        javaArgsEdit = new QLineEdit(this);
        javaArgsEdit->setPlaceholderText("Custom JVM Arguments...");

        instPathEdit = new QLineEdit(this);
        instPathEdit->setPlaceholderText("e.g. C:/RJLData/Instances/MyInstance");

        moreLayout->addRow("PermGen:", permGenEdit);
        moreLayout->addRow("Java Args (CMD):", javaArgsEdit);
        moreLayout->addRow("Instance Directory:", instPathEdit);

        moreOptionsWidget->setLayout(moreLayout);
        moreOptionsWidget->setVisible(false);
        settingsLayout->addRow(moreOptionsWidget);

        settingsGroupBox->setLayout(settingsLayout);

        lMc->addWidget(settingsGroupBox);
        lMc->addStretch(); // Push all content to the top of the tab
        stackT2->addWidget(viewMc);

        QWidget *viewPack = new QWidget();
        QVBoxLayout *lPack = new QVBoxLayout(viewPack);
        lPack->setAlignment(Qt::AlignTop);
        lPack->addWidget(new QLabel("Modpack Downloader (Select Source):"));
        auto *sourceList = new QListWidget();
        sourceList->addItems({"Modrinth", "CurseForge", "FTB"});
        lPack->addWidget(sourceList);
        lPack->addStretch();
        stackT2->addWidget(viewPack);

        QVBoxLayout *l2 = new QVBoxLayout(t2);
        l2->addWidget(stackT2);
        tabs->addTab(t2, "Setup");
        mainLayout->addWidget(tabs);

        // Persistent Navigation Buttons
        QHBoxLayout *navLayout = new QHBoxLayout();
        btnCancel = new QPushButton("Cancel", this);
        btnBack = new QPushButton("Back", this);
        btnNext = new QPushButton("Next", this);
        
        btnBack->setEnabled(false);
        btnNext->setEnabled(false);
        
        navLayout->addWidget(btnCancel);
        navLayout->addStretch();
        navLayout->addWidget(btnBack);
        navLayout->addWidget(btnNext);
        mainLayout->addLayout(navLayout);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnBack, &QPushButton::clicked, this, &InstanceWizard::onBack);
        connect(btnNext, &QPushButton::clicked, this, &InstanceWizard::onNext);

        connect(chkMinecraft, &QCheckBox::clicked, this, [this](){
            chkModpack->setChecked(false);
            updateNavButtons();
            stackT2->setCurrentIndex(0);
        });
        connect(chkModpack, &QCheckBox::clicked, this, [this](){
            chkMinecraft->setChecked(false);
            updateNavButtons();
            stackT2->setCurrentIndex(1);
        });

        auto filterRefresh = [this](bool){ populateVersionCombo(); };
        connect(chkAlpha, &QCheckBox::toggled, this, filterRefresh);
        connect(chkBeta, &QCheckBox::toggled, this, filterRefresh);
        connect(chkSnapshot, &QCheckBox::toggled, this, filterRefresh);
        
        connect(versionCombo, &QComboBox::currentTextChanged, this, [this](const QString &text){
            if (versionCombo->findText(text) != -1) {
                selectedVersion = text;
                updateInstanceName();
                if (selectedLoader != "Vanilla") fetchLoaderVersions(selectedLoader, selectedVersion);
            }
            updateNavButtons();
        });

        connect(loaderVersionCombo, &QComboBox::currentTextChanged, this, [this](){
            updateInstanceName();
            updateNavButtons();
        });

        connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &text){
            if (!instPathEdit->isModified()) instPathEdit->setText(MinecraftLauncher::getRJLDataPath() + "Instances/" + text);
            updateNavButtons();
        });

        connect(moreOptionsToggle, &QCheckBox::toggled, moreOptionsWidget, &QWidget::setVisible);
    }

    // Getters to retrieve data after dialog closes
    QString getInstanceName() const { return nameEdit->text().trimmed(); }
    QString getInstancePath() const { return instPathEdit->text().trimmed(); }
    QString getSelectedVersion() const { return selectedVersion; }
    QString getSelectedLoader() const { return selectedLoader; }
    QString getLoaderVersion() const { return loaderVersionCombo->currentText(); }
    bool isMinecraftSelected() const { return chkMinecraft->isChecked(); }
    bool isModpackSelected() const { return chkModpack->isChecked(); }

private:
    QTabWidget *tabs;
    QCheckBox *chkMinecraft, *chkModpack, *chkAlpha, *chkBeta, *chkSnapshot;
    QComboBox *versionCombo, *loaderVersionCombo;
    QLineEdit *minRamEdit, *maxRamEdit, *permGenEdit, *instPathEdit, *javaArgsEdit;
    QCheckBox *moreOptionsToggle;
    QWidget *moreOptionsWidget;
    QStackedWidget *stackT2;
    QLineEdit *nameEdit;
    QPushButton *btnNext, *btnBack, *btnCancel;
    QListWidgetItem *vanillaListItem; // To update the text of the Vanilla item
    QString selectedVersion, selectedLoader;
    QJsonArray allMcVersions;
    QNetworkAccessManager *networkManager;

    void fetchMcVersions() {
        QNetworkRequest request{QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")};
        QNetworkReply *reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                allMcVersions = QJsonDocument::fromJson(reply->readAll()).object()["versions"].toArray();
            }
            reply->deleteLater();
        });
    }

    void fetchLoaderVersions(const QString &loader, const QString &mcVersion) {
        loaderVersionCombo->clear();
        if (loader == "Vanilla" || mcVersion.isEmpty()) return;
        
        QString url;
        if (loader == "Fabric") url = QString("https://meta.fabricmc.net/v2/versions/loader/%1").arg(mcVersion);
        else if (loader == "Quilt") url = QString("https://meta.quiltmc.org/v3/versions/loader/%1").arg(mcVersion);
        
        if (url.isEmpty()) {
            loaderVersionCombo->addItem("Latest");
            return;
        }

        QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(url)));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                for (const auto &v : arr) {
                    QJsonObject obj = v.toObject();
                    if (obj.contains("loader")) {
                        loaderVersionCombo->addItem(obj["loader"].toObject()["version"].toString());
                    }
                }
            }
            reply->deleteLater();
        });
    }

    void updateInstanceName() {
        if (nameEdit->isModified()) return;
        QString prefix = selectedLoader;
        if (prefix == "Vanilla") prefix = "Minecraft";
        
        QString newName = prefix;
        if (!selectedVersion.isEmpty()) newName += " " + selectedVersion;
        
        if (selectedLoader != "Vanilla" && !loaderVersionCombo->currentText().isEmpty()) {
            newName += " (" + loaderVersionCombo->currentText() + ")";
        }
        
        nameEdit->setText(newName);
    }

    void populateVersionCombo() {
        versionCombo->clear();
        for (const auto &v : allMcVersions) {
            QJsonObject obj = v.toObject();
            QString type = obj["type"].toString();
            QString id = obj["id"].toString();

            bool show = (type == "release");
            if (chkSnapshot->isChecked() && type == "snapshot") show = true;
            if (chkBeta->isChecked() && type == "old_beta") show = true;
            if (chkAlpha->isChecked() && type == "old_alpha") show = true;

            if (show) versionCombo->addItem(id);
        }
    }

    void onNext() {
        int cur = tabs->currentIndex();
        if (cur == 1) {
            accept();
        } else {
            tabs->setCurrentIndex(cur + 1);
            updateNavButtons();
        }
    }

    void onBack() {
        int cur = tabs->currentIndex();
        if (cur > 0) {
            tabs->setCurrentIndex(cur - 1);
            updateNavButtons();
        }
    }

    void updateNavButtons() {
        int cur = tabs->currentIndex();
        btnBack->setEnabled(cur > 0);
        btnNext->setText(cur == 1 ? "Create" : "Next");
        
        bool canProceed = false;
        if (cur == 0) {
            canProceed = isMinecraftSelected() || isModpackSelected();
        } else if (cur == 1) {
            bool nameOk = !getInstanceName().isEmpty();
            bool versionOk = true;
            if (isMinecraftSelected()) {
                bool mcOk = !selectedVersion.isEmpty() && versionCombo->findText(selectedVersion) != -1;
                bool lOk = true;
                if (selectedLoader != "Vanilla") {
                    lOk = !loaderVersionCombo->currentText().isEmpty();
                }
                versionOk = mcOk && lOk;
            }
            canProceed = nameOk && versionOk;
        }
        btnNext->setEnabled(canProceed);
    }
};

QVariantMap FetchModloaderMetadata(const QString &loader, const QString &mcVersion, const QString &loaderVersion) {
    QVariantMap result;
    QString cleanLVersion = loaderVersion.split(' ').first();
    QString url;
    if (loader == "Fabric") 
        url = QString("https://meta.fabricmc.net/v2/versions/loader/%1/%2/profile/json").arg(mcVersion, cleanLVersion);
    else if (loader == "Quilt")
        url = QString("https://meta.quiltmc.org/v3/versions/loader/%1/%2/profile/json").arg(mcVersion, cleanLVersion);
    else if (loader == "NeoForge")
        result["installerUrl"] = QString("https://maven.neoforged.net/releases/net/neoforged/neoforge/%1/neoforge-%1-installer.jar").arg(cleanLVersion);
    else if (loader == "Forge")
        result["installerUrl"] = QString("https://maven.minecraftforge.net/net/minecraftforge/forge/%1-%2/forge-%1-%2-installer.jar").arg(mcVersion, cleanLVersion);

    if (url.isEmpty()) return result;

    // Retry logic for metadata fetching (5 attempts, 5s delay)
    for (int i = 0; i < 5; ++i) {
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
            result["mainClass"] = root["mainClass"].toString();
            
            QVariantList modLibs;
            QJsonArray libsArr = root["libraries"].toArray();
            for (const auto &l : libsArr) {
                QJsonObject libObj = l.toObject();
                if (libObj.contains("url")) {
                    QString baseUrl = libObj["url"].toString();
                    QString name = libObj["name"].toString();
                    QStringList parts = name.split(':');
                    QString relPath;
                    if (parts.size() >= 3) {
                        relPath = parts[0].replace('.', '/') + "/" + parts[1] + "/" + parts[2] + "/" + parts[1] + "-" + parts[2] + ".jar";
                        QVariantMap entry;
                        entry["url"] = baseUrl + relPath;
                        entry["path"] = relPath;
                        modLibs.append(entry);
                    }
                }
            }
            result["libraries"] = modLibs;
            reply->deleteLater();
            break; // Success!
        } else {
            LogLauncherEvent(QString("Metadata fetch failed (Attempt %1/5). Retrying in 5s...").arg(i+1));
            reply->deleteLater();
            if (i < 4) { QEventLoop wait; QTimer::singleShot(5000, &wait, &QEventLoop::quit); wait.exec(); }
        }
    }
    return result;
}

void ShowInstanceWizard(QWidget *parent) {
    InstanceWizard wizard(parent);
    if (wizard.exec() == QDialog::Accepted) {
        QString name = wizard.getInstanceName();
        QString selectedVersion = wizard.getSelectedVersion();
        QString loader = wizard.isMinecraftSelected() ? wizard.getSelectedLoader() : "Modpack";
        QString loaderVersion = wizard.getLoaderVersion();
        bool isMc = wizard.isMinecraftSelected();
        bool isPack = wizard.isModpackSelected();

        if (isMc && name.isEmpty()) return;
        if (isPack) name = "Downloaded_Modpack"; 

        QString targetDirPath = wizard.getInstancePath();
        QString dataRoot = MinecraftLauncher::getRJLDataPath();
        if (targetDirPath.isEmpty() || targetDirPath.startsWith("Instances/")) targetDirPath = dataRoot + "Instances/" + name;

        fs::create_directories(targetDirPath.toStdString());
        
        QVariantMap metadata;
        if (isMc && !selectedVersion.isEmpty()) {
            // Ensure legacy versions don't default to LWJGL 3 during creation
            bool isLegacyVer = (selectedVersion.startsWith("1.12") || selectedVersion.startsWith("1.8") || 
                                selectedVersion.startsWith("1.7") || selectedVersion.startsWith("1.0") ||
                                selectedVersion.contains("beta") || selectedVersion.contains("alpha"));
            
            LogLauncherEvent(QString("Preparing to download %1 for version: %2").arg(loader, selectedVersion));
            metadata = FetchVanillaMetadata(selectedVersion);

            if (loader != "Vanilla" && !loaderVersion.isEmpty()) {
                QVariantMap modMetadata = FetchModloaderMetadata(loader, selectedVersion, loaderVersion);
                if (!modMetadata.isEmpty()) {
                    QVariantList vanillaLibs = metadata["libraries"].toList();
                    QVariantList modloaderLibs = modMetadata["libraries"].toList();
                    for(int i=0; i<vanillaLibs.size(); ++i) modloaderLibs.append(vanillaLibs[i]);
                    
                    metadata["libraries"] = modloaderLibs;
                    metadata["mainClass"] = modMetadata["mainClass"].toString();
                    
                    // Force legacy LWJGL if this is an old version
                    if (isLegacyVer) metadata["lwjglVersion"] = "2.9.4";
                    
                    // Find the primary loader JAR name
                    for (const QVariant &libVar : modloaderLibs) {
                        QString libUrl = libVar.toMap()["url"].toString();
                        if (libUrl.contains(loader.toLower() + "-loader") || libUrl.contains("forge-") || libUrl.contains("neoforge-")) {
                            metadata["executableJar"] = QUrl(libUrl).fileName();
                            break;
                        }
                    }

                    metadata["loader"] = loader;
                    LogLauncherEvent("Merged " + loader + " metadata.");
                }
                
                if (modMetadata.contains("installerUrl")) {
                    metadata["installerUrl"] = modMetadata["installerUrl"];
                    LogLauncherEvent("Added installer URL to metadata: " + metadata["installerUrl"].toString());
                }
            } else if (loader != "Vanilla") {
                LogLauncherEvent("Modloader metadata fetch failed or was empty. Falling back to Vanilla.");
                QMessageBox::warning(parent, "Modloader Error", "Could not fetch modloader information. The instance will be created as Vanilla.");
            }

            if (!metadata.isEmpty()) {
                DownloadVanillaInstance(metadata, targetDirPath, selectedVersion);
                LogLauncherEvent(QString("%1 installation initiated for instance: %2").arg(loader, name));
                CreateLaunchArgs(targetDirPath, selectedVersion);
            } else {
                LogLauncherEvent("Failed to get metadata for version: " + selectedVersion);
                QMessageBox::critical(parent, "Download Error", "Could not find download URL for selected Minecraft version.");
                return;
            }
        } else {
            std::ofstream file((targetDirPath + "/minecraft.jar").toStdString());
            file.close();
        }

        // Save version info
        QFile vFile(targetDirPath + "/instance.json");
        if (vFile.open(QIODevice::WriteOnly)) {
            QJsonObject instanceInfo;
            instanceInfo["version"] = selectedVersion;
            instanceInfo["mainClass"] = metadata["mainClass"].toString();
            instanceInfo["executableJar"] = metadata.value("executableJar", QString("Minecraft %1.jar").arg(selectedVersion)).toString();

            // Store library paths for explicit classpath building in Core.cpp
            QJsonArray libsToLoad;
            QVariantList sourceLibs = metadata["libraries"].toList();
            for(const QVariant &v : sourceLibs) {
                QString p = v.toMap()["path"].toString();
                // Filter out LWJGL/JInput (handled separately by the isolated LWJGL phase)
                if (!p.isEmpty() && !p.contains("org/lwjgl") && !p.contains("jinput")) {
                     libsToLoad.append(p);
                }
            }
            instanceInfo["libraries"] = libsToLoad;
            
            // Fix: Ensure LWJGL version is never empty in json
            QString lwVer = metadata["lwjglVersion"].toString();
            if (lwVer.isEmpty()) lwVer = GetLwglVersionForMc(selectedVersion); // Fallback helper

            instanceInfo["lwjglVersion"] = lwVer;
            instanceInfo["lwjglMajor"] = metadata.value("lwjglMajor", lwVer.startsWith("3") ? 3 : 2).toInt();

            if (metadata.contains("assetIndexId")) {
                instanceInfo["assetIndex"] = metadata["assetIndexId"].toString();
            }
            vFile.write(QJsonDocument(instanceInfo).toJson());
            vFile.close();
        }

        // Create the new instance.txt for the editor
        QFile txtFile(targetDirPath + "/instance.txt");
        if (txtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&txtFile);
            out << "[ PRODUCT ]\n";
            out << "Instancename = " << name << "\n";
            out << "Minecraft = " << selectedVersion << "\n";
            out << "Java requirements = " << GetRequiredJavaMajorVersion(selectedVersion, metadata["javaMajor"].toInt()) << "\n";
            out << "LWJGL = " << metadata.value("lwjglVersion", GetLwglVersionForMc(selectedVersion)).toString() << "\n";
            out << "Group = Default\n";

            out << "\n[ Settings ]\n";
            out << "Minimum Ram = 512M\n";
            out << "Maximum Ram = 2G\n";
            out << "Permgen = 128M\n";

            out << "\n[ Tweaks ]\n";
            out << "JVM Args = \n";
            out << "Window Width = 854\n";
            out << "Window Height = 480\n";

            txtFile.close();
        }
    }
};

#include "insta_handle.moc"
