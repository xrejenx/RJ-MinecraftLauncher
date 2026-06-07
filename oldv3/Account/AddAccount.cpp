#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDesktopServices>
#include <QListWidget>
#include "webview.h"
#include <QQuickWidget>
#include <QQuickItem>
#include <QMessageBox>
#include <QIODevice>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>
#include "Account/SESSIONMANAGER/sessionman.h"
#include "Core.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;
//#include "AddAccount.h" // Bypassing broken generated header
#include "AddAccount.h" // Now that it's properly defined
/**
 * Internal Class Declarations (Headerless)
 */

// Mark as static to avoid linker conflicts with offline_handle.cpp
static bool SaveOfflineAccount(const QString &username) {
    QJsonObject acc;
    acc["Username"] = username;
    // Generate a simple random UUID for offline use
    acc["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces).replace("-", "");
    acc["accessToken"] = "0";
    acc["userType"] = "offline";

    QString dataPath = MinecraftLauncher::getRJLDataPath() + "userdata/OFFLINE";
    QDir().mkpath(dataPath);

    QFile file(dataPath + "/" + username + ".json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(acc).toJson());
        file.close();
        return true;
    }
    return false;
}

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

        view = nullptr;

#ifdef Q_OS_WIN
        // Windows-specific WebView2 implementation goes here.
        // For now, we stub it to allow compilation without the QtWebView module.
        layout->addWidget(new QLabel("Native WebView2 Controller (Edge Engine)", this));
#else
        view = new QQuickWidget(this);
        view->setResizeMode(QQuickWidget::SizeRootObjectToView);

        QByteArray qss = "import QtQuick; import QtWebView; "
                         "WebView { "
                         "  id: webView; "
                         "  anchors.fill: parent; "
                         "  onUrlChanged: root.urlChanged(url); "
                         "  signal urlChanged(url newUrl); "
                         "}";

        view->setSource(QUrl("data:text/plain;base64," + qss.toBase64()));
        layout->addWidget(view);

        if (view->rootObject()) {
            connect(view->rootObject(), SIGNAL(urlChanged(QUrl)), this, SLOT(onUrlChanged(QUrl)));
        }
#endif

        std::string authUrl = "https://login.live.com/oauth20_authorize.srf"
                              "?client_id=" + CLIENT_ID +
                              "&response_type=code" +
                              "&redirect_uri=" + REDIRECT_URI +
                              "&scope=" + SCOPE +
                              "&prompt=select_account";
        
        if (view && view->rootObject()) {
            view->rootObject()->setProperty("url", QUrl(QString::fromStdString(authUrl)));
        }
    }

private:
    QQuickWidget *view;

    void onUrlChanged(const QUrl &url) {
        if (url.toString().startsWith(QString::fromStdString(REDIRECT_URI))) {
            QUrlQuery query(url);
            QString code = query.queryItemValue("code");
            if (!code.isEmpty()) {
                if (view) view->setEnabled(false);
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
            QString dataPath = MinecraftLauncher::getRJLDataPath() + "userdata/MICROSOFT";
            QDir().mkpath(dataPath);
            
            QFile file(dataPath + "/" + profileName + ".json");
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(acc).toJson());
                file.close();
            }

            LogLauncherEvent("Successfully logged in as " + profileName);
            QMessageBox::information(this, "Success", "Login successful as " + profileName);
            SessionManager::instance().saveSession(QJsonDocument(acc).toJson());
            accept();
        });

        connect(flow, &MicrosoftAuthFlow::failed, this, [this](const QString &err) {
            LogLauncherEvent("MSA Login Failed: " + err);
            QMessageBox::critical(this, "Login Failed", err);
            if (view) view->setEnabled(true);
        });

        flow->start(code);
    }
};

class OfflineLoginDialog : public QDialog {
public:
    OfflineLoginDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add Offline Account");
        setFixedSize(350, 150);
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
    QString getUsername() const { return userEdit->text().trimmed(); }
private:
    QLineEdit *userEdit;
};

void ShowAddMicrosoftAccountDialog(QWidget *parent) { MicrosoftLoginDialog(parent).exec(); }
void ShowAddElyByAccountDialog(QWidget *parent) { QMessageBox::information(parent, "Ely.By", "Not implemented yet"); }
QString ShowAddOfflineAccountDialog(QWidget *parent) {
    OfflineLoginDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) return dlg.getUsername();
    return QString();
}

AddAccount::AddAccount(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Account Manager");
    setFixedSize(400, 450);

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
    layout->addWidget(accountList);

    // Connections for adding accounts
    connect(msBtn, &QPushButton::clicked, this, [this]() { ShowAddMicrosoftAccountDialog(this); refreshAccounts(); });
    connect(elyBtn, &QPushButton::clicked, this, [this]() { ShowAddElyByAccountDialog(this); refreshAccounts(); });
    connect(offBtn, &QPushButton::clicked, this, [this]() { 
        QString name = ShowAddOfflineAccountDialog(this); 
        refreshAccounts(); 
        if (!name.isEmpty()) {
            auto items = accountList->findItems(name, Qt::MatchExactly);
            if (!items.isEmpty()) accountList->setCurrentItem(items.first());
        }
    });

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
            SessionManager::instance().saveSession(file.readAll());
            file.close();
            accept();
        }
    });

    refreshAccounts();
}

void AddAccount::refreshAccounts() {
    accountList->clear();
    std::vector<std::string> providers = {"MICROSOFT", "OFFLINE", "ELYBY"};
    for (const auto& provider : providers) {
        QString path = MinecraftLauncher::getRJLDataPath() + "userdata/" + QString::fromStdString(provider);
        QDir dir(path);
        if (dir.exists()) {
            for (const auto& entry : dir.entryInfoList({"*.json"}, QDir::Files)) {
                    QString name = entry.baseName();
                    QListWidgetItem *item = new QListWidgetItem(name, accountList);
                    item->setData(Qt::UserRole, entry.absoluteFilePath());
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
