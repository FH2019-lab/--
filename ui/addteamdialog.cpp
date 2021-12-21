#include "addteamdialog.h"
#include "ui_addteamdialog.h"

AddTeamDialog::AddTeamDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddTeamDialog)
{
    ui->setupUi(this);
}

void AddTeamDialog::accept(){
    emit newTeamAdded(ui->lineEdit->text());
    QDialog::accept();
}

AddTeamDialog::~AddTeamDialog()
{
    delete ui;
}
