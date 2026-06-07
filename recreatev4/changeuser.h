#ifndef CHANGEUSER_H
#define CHANGEUSER_H

#include <QDialog>

namespace Ui {
class ChangeUser;
}

class ChangeUser : public QDialog   // <-- inherit QDialog
{
    Q_OBJECT

public:
    explicit ChangeUser(QWidget *parent = nullptr);
    ~ChangeUser();

private:
    Ui::ChangeUser *ui;
};

#endif // CHANGEUSER_H