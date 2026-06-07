// mainwindow.h (additions)
#include "ui_mainwindow.h"
#include "updater.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // existing getters...
    QListWidget* remoteVersionsList() const;
    QListWidget* filesToDownloadList() const;
    QTextEdit*   releaseDescription() const;
    QLabel*      currentBuildLabel() const;
    QPushButton* downloadButton() const;

private slots:
    void on_ChangeUser_bta_clicked();
    void on_Setting_clicked();
    void on_downloadButton_clicked();            // new slot for Download and Install
    void onSevenZipDownloaded();                 // helper
    void onZipDownloaded();                      // helper
    void onNetworkError(QNetworkReply::NetworkError);

private:
    Ui::MainWindow *ui;
    Updater *m_updater;
    QNetworkAccessManager *m_netManager;        // new
    QNetworkReply *m_currentReply;              // new
    QString m_downloadedZipDir;                 // new
    QString m_sevenZipDir;                      // new
    QString m_sevenZipExePath;                  // new
    QString m_lastDownloadedZipPath;            // new
};
