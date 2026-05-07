#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDesktopServices>
#include <QListWidget>
#include <QWebEngineView>
#include <QMessageBox>
#include <QIODevice>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;
#include "AddAccount.h"

/**
 * Internal Class Declarations (Headerless)
 */

bool SaveOfflineAccount(const QString &username);

class MicrosoftAuthFlow : public QObject {
    Q_OBJECT
public:
    explicit MicrosoftAuthFlow(QObject *parent = nullptr);
    void start(const QString &code);
signals:
    void progressMessage(const QString &msg);
    void success(const QJsonObject &acc);
    void failed(const QString &err);
};

MicrosoftAuthFlow::MicrosoftAuthFlow(QObject *parent) : QObject(parent) {}

void MicrosoftAuthFlow::start(const QString &code) {
    emit progressMessage("Starting token exchange with code: " + code);
    // Implementation of the OAuth/Microsoft Auth flow logic goes here.
}

// External logger defined in ConsoleOutput.cpp
void LogLauncherEvent(const QString &message);

const std::string CLIENT_ID = "00000000402b5328";
const std::string REDIRECT_URI = "https://login.live.com/oauth20_desktop.srf";
const std::string SCOPE = "XboxLive.signin offline_access";

class MicrosoftLoginDialog : public QDialog {
    Q_OBJECT
public:
    MicrosoftLoginDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Microsoft Login");
        setFixedSize(600, 600);

        QVBoxLayout *layout = new QVBoxLayout(this);
        
        view = new QWebEngineView(this);
        layout->addWidget(view);

        connect(view, &QWebEngineView::urlChanged, this, &MicrosoftLoginDialog::onUrlChanged);

        std::string authUrl = "https://login.live.com/oauth20_authorize.srf"
                              "?client_id=" + CLIENT_ID +
                              "&response_type=code" +
                              "&redirect_uri=" + REDIRECT_URI +
                              "&scope=" + SCOPE +
                              "&prompt=select_account";
        
        view->setUrl(QUrl(QString::fromStdString(authUrl)));
    }

private:
    QWebEngineView *view;

    void onUrlChanged(const QUrl &url) {
        if (url.toString().startsWith(QString::fromStdString(REDIRECT_URI))) {
            QUrlQuery query(url);
            QString code = query.queryItemValue("code");
            if (!code.isEmpty()) {
                view->setEnabled(false); // Stop interaction while processing
                processTokenExchange(code);
            }
        }
    }

    void processTokenExchange(const QString& code) {
        auto flow = new MicrosoftAuthFlow(this);
        
        connect(flow, &MicrosoftAuthFlow::progressMessage, this, [](const QString &msg) {
            LogLauncherEvent(msg);
        });

        connect(flow, &MicrosoftAuthFlow::success, this, [this](const QJsonObject &acc) {
            QString profileName = acc["Username"].toString();
            fs::create_directories("userdata/MICROSOFT");
            std::ofstream file("userdata/MICROSOFT/" + profileName.toStdString() + ".json");
            file << QJsonDocument(acc).toJson().toStdString();
            file.close();

            LogLauncherEvent("Successfully logged in as " + profileName);
            QMessageBox::information(this, "Success", "Login successful as " + profileName);
            accept();
        });

        connect(flow, &MicrosoftAuthFlow::failed, this, [this](const QString &err) {
            LogLauncherEvent("MSA Login Failed: " + err);
            QMessageBox::critical(this, "Login Failed", err);
            view->setEnabled(true);
        });

        flow->start(code);
    }
};

class OfflineLoginDialog : public QDialog {
public:
    OfflineLoginDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add Offline Account");
        setFixedSize(350, 150);
        setStyleSheet("background-color: white; color: black;");
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Username:"));
        userEdit = new QLineEdit(this);
        layout->addWidget(userEdit);
        QPushButton *addBtn = new QPushButton("Login", this);
        layout->addWidget(addBtn);
        connect(addBtn, &QPushButton::clicked, this, [this]() {
            QString name = userEdit->text().trimmed();
            if (name.isEmpty()) return;
            
            if (SaveOfflineAccount(name)) this->accept();
        });
    }
private:
    QLineEdit *userEdit;
};

void ShowAddMicrosoftAccountDialog(QWidget *parent) { MicrosoftLoginDialog(parent).exec(); }
void ShowAddElyByAccountDialog(QWidget *parent) { QMessageBox::information(parent, "Ely.By", "Not implemented yet"); }
void ShowAddOfflineAccountDialog(QWidget *parent) { OfflineLoginDialog(parent).exec(); }

AddAccount::AddAccount(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Account Manager");
    setFixedSize(400, 450);
    setStyleSheet("background-color: white; color: black;");

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Top Buttons for adding accounts
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *msBtn = new QPushButton("Microsoft", this);
    QPushButton *elyBtn = new QPushButton("Ely.by", this);
    QPushButton *offBtn = new QPushButton("Offline", this);
    topLayout->addWidget(msBtn);
    topLayout->addWidget(elyBtn);
    topLayout->addWidget(offBtn);
    layout->addLayout(topLayout);

    layout->addWidget(new QLabel("Select active account:", this));

    // Account list box
    accountList = new QListWidget(this);
    accountList->setStyleSheet("border: 1px solid #ccc; background: #f9f9f9; color: black;");
    layout->addWidget(accountList);

    // Connections for adding accounts
    connect(msBtn, &QPushButton::clicked, this, [this]() { ShowAddMicrosoftAccountDialog(this); refreshAccounts(); });
    connect(elyBtn, &QPushButton::clicked, this, [this]() { ShowAddElyByAccountDialog(this); refreshAccounts(); });
    connect(offBtn, &QPushButton::clicked, this, [this]() { ShowAddOfflineAccountDialog(this); refreshAccounts(); });

    // Action buttons at the bottom
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QPushButton *applyBtn = new QPushButton("Apply", this);
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    bottomLayout->addStretch();
    bottomLayout->addWidget(applyBtn);
    bottomLayout->addWidget(cancelBtn);
    layout->addLayout(bottomLayout);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = accountList->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Please select an account from the list.");
            return;
        }

        QString filePath = item->data(Qt::UserRole).toString();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QFile sessionFile("LauncherSession.json");
            if (sessionFile.open(QIODevice::WriteOnly)) {
                sessionFile.write(data);
                sessionFile.close();
                accept();
            }
        }
    });

    refreshAccounts();
}

void AddAccount::refreshAccounts() {
    accountList->clear();
    std::vector<std::string> providers = {"MICROSOFT", "OFFLINE", "ELYBY"};
    for (const auto& provider : providers) {
        std::string path = "userdata/" + provider;
        if (fs::exists(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.path().extension() == ".json") {
                    QString name = QString::fromStdString(entry.path().stem().string());
                    QListWidgetItem *item = new QListWidgetItem(name, accountList);
                    item->setData(Qt::UserRole, QString::fromStdString(entry.path().string()));
                }
            }
        }
    }
}

void ShowProfileWindow(QWidget *parent) {
    // Implementation moved here to remove dependency on external headers
    AddAccount dialog(parent);
    dialog.exec();
    LogLauncherEvent("Account management window closed.");
}

#include "AddAccount.moc"