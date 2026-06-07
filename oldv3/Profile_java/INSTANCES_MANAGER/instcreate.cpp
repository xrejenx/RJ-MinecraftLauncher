#include "instcreate.h"
#include "Core.h" // For MinecraftLauncher::getRJLDataPath, LogLauncherEvent, RefreshMainWindowUI
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QFileInfo> // Added for QFileInfo
#include <QDialog>

extern void LogLauncherEvent(const QString &message);
void Insta_prefix_vanillamc_maker(const QString &path, const QString &version); // Ensure this matches vanilla_installer.cpp exactly

// External function to refresh main window UI (declared in Core.h/cpp)
extern void RefreshMainWindowUI();

InstCreate::InstCreate(QObject *parent) : QObject(parent) {}

void InstCreate::finalizeInstance(const QString &instanceName, const QString &versionId,
                                  const QString &javaVersionPath, bool mcDownloadSuccess, bool lwjglDownloadSuccess,
                                  QWidget *parentWidget) {
    LogLauncherEvent("InstCreate::finalizeInstance: Finalizing instance " + instanceName); //

    QString targetDirPath = MinecraftLauncher::getRJLDataPath() + "Instances/" + instanceName; //
    Insta_prefix_vanillamc_maker(targetDirPath, versionId); //

    QDialog resultDialog(parentWidget); //
    resultDialog.setWindowTitle("Instance Creation Summary"); //
    resultDialog.setFixedSize(400, 250); //
    QVBoxLayout *layout = new QVBoxLayout(&resultDialog); //

    QLabel *titleLabel = new QLabel("<h2>Instance Creation Summary</h2>", &resultDialog); //
    titleLabel->setAlignment(Qt::AlignCenter); //
    layout->addWidget(titleLabel); //

    QString statusText = "<b>Minecraft Version:</b> " + versionId + "<br>";
    statusText += QString("<b>Java Version:</b> %1<br>").arg(javaVersionPath.isEmpty() ? "Not Found" : QFileInfo(javaVersionPath).dir().dirName());
    statusText += QString("<b>Minecraft Download:</b> %1<br>").arg(mcDownloadSuccess ? "<font color='green'>Success</font>" : "<font color='red'>Failed</font>");
    statusText += QString("<b>LWJGL Download:</b> %1<br>").arg(lwjglDownloadSuccess ? "<font color='green'>Success</font>" : "<font color='red'>Failed</font>");

    QLabel *summaryLabel = new QLabel(statusText, &resultDialog); //
    summaryLabel->setAlignment(Qt::AlignLeft); //
    layout->addWidget(summaryLabel); //

    layout->addStretch(); //

    QPushButton *okButton = new QPushButton("OK", &resultDialog); //
    layout->addWidget(okButton, 0, Qt::AlignCenter); //
    QObject::connect(okButton, &QPushButton::clicked, &resultDialog, &QDialog::accept); //

    resultDialog.exec(); //

    // Directly call refresh functions (assuming they are globally accessible or take appropriate arguments)
    RefreshMainWindowUI(); // This function needs to be implemented in Core.cpp //
    emit instanceCreationFinished(); // Signal to notify main window to refresh instance list
    LogLauncherEvent("InstCreate::finalizeInstance: Instance " + instanceName + " creation process finished."); //
}
