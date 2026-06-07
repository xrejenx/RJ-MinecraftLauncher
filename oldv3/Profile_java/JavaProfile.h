#ifndef JAVAPROFILE_H
#define JAVAPROFILE_H

#include <QDialog>

class QListWidget;

class JavaProfileWindow : public QDialog {
    Q_OBJECT
public:
    explicit JavaProfileWindow(QWidget *parent = nullptr);
    ~JavaProfileWindow();

signals:
    void profileChanged();

private:
    void refreshInstances();
    QListWidget *instanceList;
};

void ShowJavaProfileWindow(QWidget *parent);

#endif // JAVAPROFILE_H