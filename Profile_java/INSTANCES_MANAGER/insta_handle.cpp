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
void DownloadVanillaInstance(const QVariantMap &metadata, const QString &targetDirPath, const QString &versionId);
void CreateLaunchArgs(const QString &targetDirPath, const QString &versionId);

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
        
        versionCombo = new QComboBox(this);
        versionCombo->setPlaceholderText("Select Minecraft Version...");
        versionCombo->setEnabled(false);
        versionCombo->setFixedHeight(35);
        versionCombo->setEditable(true);
        versionCombo->setInsertPolicy(QComboBox::NoInsert);

        if (versionCombo->completer()) {
            versionCombo->completer()->setFilterMode(Qt::MatchContains);
            versionCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
        }

        connect(loaderList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
            selectedLoader = item->text();
            bool isVanilla = selectedLoader.startsWith("Vanilla");
            versionCombo->setEnabled(isVanilla);
            if (isVanilla) {
                populateVersionCombo();
            } else {
                versionCombo->clear();
            }
            updateInstanceName();
        });

        QHBoxLayout *selectionRow = new QHBoxLayout();
        selectionRow->setSpacing(5);
        selectionRow->addWidget(loaderList, 2);
        selectionRow->addWidget(versionCombo, 3, Qt::AlignTop); // Add the version dropdown
        lMc->addLayout(selectionRow); // Add the selection row to the main tab layout

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
        instPathEdit->setPlaceholderText("e.g. Instances/MyInstance");

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
            }
            updateNavButtons();
        });

        connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &text){
            if (!instPathEdit->isModified()) instPathEdit->setText("Instances/" + text);
            updateNavButtons();
        });

        connect(moreOptionsToggle, &QCheckBox::toggled, moreOptionsWidget, &QWidget::setVisible);
    }

    // Getters to retrieve data after dialog closes
    QString getInstanceName() const { return nameEdit->text().trimmed(); }
    QString getInstancePath() const { return instPathEdit->text().trimmed(); }
    QString getSelectedVersion() const { return selectedVersion; }
    bool isMinecraftSelected() const { return chkMinecraft->isChecked(); }
    bool isModpackSelected() const { return chkModpack->isChecked(); }

private:
    QTabWidget *tabs;
    QCheckBox *chkMinecraft, *chkModpack, *chkAlpha, *chkBeta, *chkSnapshot;
    QComboBox *versionCombo;
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

    void updateInstanceName() {
        if (nameEdit->isModified()) return;
        QString prefix = selectedLoader;
        if (prefix == "Vanilla") prefix = "Minecraft";
        
        QString newName = prefix;
        if (!selectedVersion.isEmpty()) newName += " " + selectedVersion;
        
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
                versionOk = !selectedVersion.isEmpty() && versionCombo->findText(selectedVersion) != -1;
            }
            canProceed = nameOk && versionOk;
        }
        btnNext->setEnabled(canProceed);
    }
};

void ShowInstanceWizard(QWidget *parent) {
    InstanceWizard wizard(parent);
    if (wizard.exec() == QDialog::Accepted) {
        QString name = wizard.getInstanceName();
        QString selectedVersion = wizard.getSelectedVersion();
        bool isMc = wizard.isMinecraftSelected();
        bool isPack = wizard.isModpackSelected();

        if (isMc && name.isEmpty()) return;
        if (isPack) name = "Downloaded_Modpack"; 

        QString targetDirPath = wizard.getInstancePath();
        if (targetDirPath.isEmpty()) targetDirPath = "Instances/" + name;

        fs::create_directories(targetDirPath.toStdString());
        
        QVariantMap metadata;
        if (isMc && !selectedVersion.isEmpty()) {
            LogLauncherEvent("Preparing to download Minecraft JAR for version: " + selectedVersion);
            metadata = FetchVanillaMetadata(selectedVersion);
            if (!metadata.isEmpty()) {
                DownloadVanillaInstance(metadata, targetDirPath, selectedVersion);
                LogLauncherEvent("Vanilla installation initiated for instance: " + name);
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
            if (metadata.contains("assetIndexId")) {
                instanceInfo["assetIndex"] = metadata["assetIndexId"].toString();
            }
            vFile.write(QJsonDocument(instanceInfo).toJson());
            vFile.close();
        }
    }
};
#include "insta_handle.moc"