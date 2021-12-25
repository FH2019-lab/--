#include "mergedialog.h"
#include "ui_mergedialog.h"

MergeDialog::MergeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MergeDialog)
{
    ui->setupUi(this);
}

MergeDialog::~MergeDialog()
{
    delete ui;
}

void MergeDialog::setPlayerList(QStringList list){
    ui->comboBox->addItem("");
    ui->comboBox->addItems(list);
}

void MergeDialog::accept(){
    QDialog::done(ui->comboBox->currentText().toInt());
}

void MergeDialog::on_comboBox_currentTextChanged(const QString &arg1)
{
    if (arg1 == ""){
        ui->label_name->clear();
        ui->label_team->clear();
        ui->label_number->clear();
        ui->label_picture->clear();
        return;
    }
    int idx = arg1.toInt();
    QString name, team, number;
    QImage pic;
    emit getInfo(idx, name, team, number, pic);
    ui->label_name->setText(name);
    ui->label_team->setText(team);
    ui->label_number->setText(number);
    if (pic.isNull())
        return;
    double scale= std::min(ui->label_picture->width()*1.0/pic.width(), ui->label_picture->height()*1.0/pic.height());
    pic = pic.scaled(int(pic.width()*scale), int(pic.height()*scale));
    ui->label_picture->setPixmap(QPixmap::fromImage(pic));
}

