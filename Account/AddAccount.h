#ifndef ADDACCOUNT_H
#define ADDACCOUNT_H

#include <QDialog>
#include <QListWidget>

/**
 * AddAccount.h - Header for account management
 */

class AddAccount : public QDialog {
    Q_OBJECT
public:
    explicit AddAccount(QWidget *parent = nullptr);

private:
    QListWidget *accountList;
    void refreshAccounts();
};

void ProcessAccountLogin(const QString &username);
#endif // ADDACCOUNT_H