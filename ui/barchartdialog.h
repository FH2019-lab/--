#ifndef BARCHARTDIALOG_H
#define BARCHARTDIALOG_H

#include <QDialog>
#include <QButtonGroup>
#include <QtCharts>

namespace Ui {
class BarChartDialog;
}

class BarChartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BarChartDialog(QWidget *parent = nullptr);
    ~BarChartDialog();

signals:
    void getPlayersInfo(QString, QList<int>&, QList<qreal>&);

private:
    Ui::BarChartDialog *ui;

    QButtonGroup* buttonGroup;
    void updateChart(int);

    QChart* chart;
};

#endif // BARCHARTDIALOG_H
