#ifndef MERGEDIALOG_H
#define MERGEDIALOG_H

#include <QDialog>

namespace Ui {
class MergeDialog;
}

class MergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MergeDialog(QWidget *parent = nullptr);
    ~MergeDialog();

protected:
    void accept();

signals:
    void getInfo(int idx, QString&, QString&, QString&, QImage&);

public slots:
    void setPlayerList(QStringList);

private slots:
    void on_comboBox_currentTextChanged(const QString &arg1);

private:
    Ui::MergeDialog *ui;
};

#endif // MERGEDIALOG_H
