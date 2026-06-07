
#include "changeuser.h"
#include "ui_changeuser.h"

ChangeUser::ChangeUser(QWidget *parent)
    : QDialog(parent)   // <-- call QDialog constructor
    , ui(new Ui::ChangeUser)
{
    ui->setupUi(this);
    // your setup code...
}

ChangeUser::~ChangeUser()
{
    delete ui;
}
