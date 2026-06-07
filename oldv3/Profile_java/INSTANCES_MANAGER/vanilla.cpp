#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QLabel>

/**
 * vanilla.cpp - Handles Minecraft version selection from official Mojang manifest
 */

class VanillaVersionDialog : public QDialog {
    Q_OBJECT
public:
    explicit VanillaVersionDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Select Minecraft Version");
        setFixedSize(450, 550);

        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("<b>Official Minecraft Versions</b>"));

        searchEdit = new QLineEdit(this);
        searchEdit->setPlaceholderText("Search version (e.g. 1.21)...");
        connect(searchEdit, &QLineEdit::textChanged, this, [this](){ populateList(); });
        layout->addWidget(searchEdit);

        auto *filterLayout = new QHBoxLayout();
        cbSnapshots = new QCheckBox("Snapshots", this);
        cbBetas = new QCheckBox("Beta", this);
        cbAlphas = new QCheckBox("Alpha", this);

        filterLayout->addWidget(cbSnapshots);
        filterLayout->addWidget(cbBetas);
        filterLayout->addWidget(cbAlphas);
        layout->addLayout(filterLayout);

        versionList = new QListWidget(this);
        layout->addWidget(versionList);

        auto *btnLayout = new QHBoxLayout();
        auto *okBtn = new QPushButton("Select Version", this);
        btnLayout->addStretch();
        btnLayout->addWidget(okBtn);
        layout->addLayout(btnLayout);

        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
        
        auto refresh = [this](bool){ populateList(); };
        connect(cbSnapshots, &QCheckBox::toggled, this, refresh);
        connect(cbBetas, &QCheckBox::toggled, this, refresh);
        connect(cbAlphas, &QCheckBox::toggled, this, refresh);

        networkManager = new QNetworkAccessManager(this);
        fetchVersions();
    }

    QString getSelectedVersion() {
        if (auto *item = versionList->currentItem()) return item->text();
        return QString();
    }

private:
    QListWidget *versionList;
    QLineEdit *searchEdit;
    QCheckBox *cbSnapshots, *cbBetas, *cbAlphas;
    QNetworkAccessManager *networkManager;
    static QJsonArray allVersions; // Static cache for speed

    void fetchVersions() {
        if (!allVersions.isEmpty()) {
            populateList();
            return;
        }

        QNetworkRequest request{QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")};
        QNetworkReply *reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                allVersions = doc.object()["versions"].toArray();
                populateList();
            }
            reply->deleteLater();
        });
    }

    void populateList() {
        versionList->clear();
        QString query = searchEdit->text().trimmed();

        for (const auto &v : allVersions) {
            QJsonObject obj = v.toObject();
            QString type = obj["type"].toString();
            QString id = obj["id"].toString();

            if (!query.isEmpty() && !id.contains(query, Qt::CaseInsensitive)) continue;

            bool show = (type == "release");
            if (cbSnapshots->isChecked() && type == "snapshot") show = true;
            if (cbBetas->isChecked() && type == "old_beta") show = true;
            if (cbAlphas->isChecked() && type == "old_alpha") show = true;

            if (show) versionList->addItem(id);
        }
    }
};

QString ShowVanillaVersionSelector(QWidget *parent) {
    VanillaVersionDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.getSelectedVersion();
    }
    return QString();
}

QJsonArray VanillaVersionDialog::allVersions = QJsonArray();

#include "vanilla.moc"
