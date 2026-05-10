#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDirIterator>

#include "LauncherUpdater/LauncherDownload/downloadzip.h" // For ExtractZipFile

// External logger (from ConsoleOutput.cpp)
void LogLauncherEvent(const QString &message);

class MadeChangesDialog : public QDialog {
    Q_OBJECT
public:
    explicit MadeChangesDialog(const QString &backupZipToRestore = "", QWidget *parent = nullptr)
        : QDialog(parent), m_backupZipToRestore(backupZipToRestore) {
        setWindowTitle("RJ Launcher Rollback / Maintenance");
        setFixedSize(500, 400);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel("<h2>RJ Launcher Rollback</h2>", this));
        mainLayout->addWidget(new QLabel("Select a backup to restore or perform maintenance:", this));

        m_backupList = new QListWidget(this);
        mainLayout->addWidget(m_backupList);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        m_rollbackButton = new QPushButton("Restore Selected Backup", this);
        m_cancelButton = new QPushButton("Cancel", this);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_rollbackButton);
        buttonLayout->addWidget(m_cancelButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_rollbackButton, &QPushButton::clicked, this, &MadeChangesDialog::performRollback);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        populateBackupList();

        // If a specific backup was passed as argument, select it and initiate rollback
        if (!m_backupZipToRestore.isEmpty()) {
            for (int i = 0; i < m_backupList->count(); ++i) {
                if (m_backupList->item(i)->text() == m_backupZipToRestore) {
                    m_backupList->setCurrentRow(i);
                    performRollback();
                    break;
                }
            }
        }
    }

private slots:
    void populateBackupList() {
        m_backupList->clear();
        QDir oldDir("C:/RJLauncherData/oldlauncher"); // Look in the data root
        if (!oldDir.exists()) {
            m_backupList->addItem("No backups found.");
            m_rollbackButton->setEnabled(false);
            return;
        }

        QStringList backups = oldDir.entryList(QStringList() << "RJML.*.zip", QDir::Files, QDir::Time | QDir::Reversed);
        if (backups.isEmpty()) {
            m_backupList->addItem("No backups found.");
            m_rollbackButton->setEnabled(false);
        } else {
            m_backupList->addItems(backups);
            m_rollbackButton->setEnabled(true);
        }
    }

    void performRollback() {
        QListWidgetItem *selectedItem = m_backupList->currentItem();
        if (!selectedItem) {
            QMessageBox::warning(this, "No Selection", "Please select a backup to restore.");
            return;
        }

        QString backupFileName = selectedItem->text();
        QString backupZipPath = QDir("C:/RJLauncherData/oldlauncher").absoluteFilePath(backupFileName);
        QString installPath = "C:/Program Files/RJLauncher";

        if (QMessageBox::question(this, "Confirm Rollback",
                                  QString("Are you sure you want to restore '%1'? This will overwrite your current installation.").arg(backupFileName),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }

        // 1. Wipe the current installation (except oldlauncher and Tools)
        QDir appDir(installPath);
        if (appDir.exists()) {
            QDirIterator it(installPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            while (it.hasNext()) {
                QString entry = it.next();
                QFileInfo info(entry);
                if (info.fileName() == "oldlauncher" || info.fileName() == "Tools") continue; // Don't delete these
                if (info.isDir()) QDir(entry).removeRecursively();
                else QFile::remove(entry);
            }
        }

        // 2. Extract the selected backup
        if (ExtractZipFile(backupZipPath, installPath)) {
            QMessageBox::information(this, "Rollback Complete", "RJ Launcher has been restored. It will now restart.");
            QProcess::startDetached(QDir(installPath).absoluteFilePath("RJML.exe"));
            QCoreApplication::quit();
        } else {
            QMessageBox::critical(this, "Rollback Failed", "Failed to extract the backup. Your installation might be corrupted.");
        }
    }

private:
    QListWidget *m_backupList;
    QPushButton *m_rollbackButton;
    QPushButton *m_cancelButton;
    QString m_backupZipToRestore;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QString backupArg;
    if (argc > 1) backupArg = argv[1];
    MadeChangesDialog w(backupArg);
    w.show();
    return a.exec();
}

#include "MadeChanges.moc"