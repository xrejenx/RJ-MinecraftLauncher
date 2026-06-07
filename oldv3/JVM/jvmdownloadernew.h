#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QCheckBox>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QNetworkReply>

class JVMDownloaderNew; // Forward declaration

// Custom QListWidgetItem to hold version info and a download button/progress bar
class JavaVersionItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit JavaVersionItemWidget(const QString &versionText, QWidget *parent = nullptr);
    void setProgress(int value, const QString &text = QString());
    void showDownloadState(bool downloading); // True for progress bar, false for download button
    void setInstalled(bool installed); // Greys out and disables selection
    QLabel *versionLabel; // Public to allow direct access for text updates
    QProgressBar *progressBar;
    QListWidgetItem *listItem; // Pointer back to the QListWidgetItem this widget is for
    QCheckBox *checkBox; // Checkbox for multi-selection

signals:
    void itemChecked(QListWidgetItem*, bool); // Emitted when checkbox state changes

private:
    QString m_versionText;
};

class JVMDownloaderNew : public QDialog {
    Q_OBJECT
public:
    explicit JVMDownloaderNew(QWidget *parent = nullptr);
    ~JVMDownloaderNew() override;

private slots:
    void fetchAll();
    void fetchAzulVersions();
    void onDownloadSelectedClicked(); // Slot for the bottom download button
    void updateDownloadButtonState(); // Helper to enable/disable the button
    void onItemClicked(QListWidgetItem* item); // Toggle checkbox on row click
private:
    QListWidget* currentSelectedList();

    QCheckBox *cbShowAll, *cbShowOld;
    QTabWidget *tabs;
    QListWidget *azulList;
    QNetworkAccessManager *networkManager;
    QNetworkReply *m_currentFetchReply = nullptr; // Track current request
    QPushButton *downloadSelectedButton; // Bottom button
    QJsonArray allAzulVersions;
};

