#ifndef ADDACCOUNT_H
#define ADDACCOUNT_H

#include <QDialog>
#include <QListWidget> // Required for member declaration

class AddAccount : public QDialog {
    Q_OBJECT
public:
    explicit AddAccount(QWidget *parent = nullptr);
private:
    void refreshAccounts();
    QListWidget *accountList;
};
#endif