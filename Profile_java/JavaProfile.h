#ifndef JAVAPROFILE_H
#define JAVAPROFILE_H
/**
 * JavaProfile.h - Instance and Profile Manager
 */

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <filesystem>
#include <QComboBox> // Added for PopulateInstanceList signature

// Forward declarations for UI functions defined in other components
void ShowInstanceWizard(QWidget *parent);
void ShowInstanceSettings(QWidget *parent, const QString &instanceName);
void ShowJavaSettingsWindow(QWidget *parent);
void PopulateInstanceList(QComboBox *comboBox);

// External function from ConsoleOutput.cpp
void LogLauncherEvent(const QString &message);
void LogWindowContent(QWidget *window);

class JavaProfileWindow : public QDialog {
    Q_OBJECT
public:
    explicit JavaProfileWindow(QWidget *parent = nullptr);
    virtual ~JavaProfileWindow();

private:
    QListWidget *instanceList;
    void refreshInstances();
};

void ShowJavaProfileWindow(QWidget *parent);

#endif // JAVAPROFILE_H