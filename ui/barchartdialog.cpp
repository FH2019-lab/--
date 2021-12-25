#include "barchartdialog.h"
#include "ui_barchartdialog.h"

BarChartDialog::BarChartDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BarChartDialog)
{
    ui->setupUi(this);

    //设置radio button grroup
    buttonGroup = new QButtonGroup;
    buttonGroup->addButton(ui->radioButton_distance);
    buttonGroup->addButton(ui->radioButton_speed_avg);
    buttonGroup->addButton(ui->radioButton_speed_max);
    buttonGroup->setId(ui->radioButton_speed_max, 0);
    buttonGroup->setId(ui->radioButton_speed_avg, 1);
    buttonGroup->setId(ui->radioButton_distance, 2);
    connect(buttonGroup, &QButtonGroup::idClicked, this, &BarChartDialog::updateChart);

    chart = new QChart;
    ui->chartView->setChart(chart);
}

BarChartDialog::~BarChartDialog()
{
    delete ui;
}

void BarChartDialog::updateChart(int type){
    //清空
    auto series_old = chart->series();
    for (auto s:series_old)
        chart->removeSeries(s);
    auto axes_old = chart->axes(Qt::Horizontal);
    for (auto a:axes_old)
        chart->removeAxis(a);
    axes_old = chart->axes(Qt::Vertical);
    for (auto a:axes_old)
        chart->removeAxis(a);

    QList<int> id;
    QList<qreal> data;
    QString typeName;
    switch (type) {
    case 0: //max
        typeName = "MaxSpeed";
        break;
    case 1:
        typeName = "AvgSpeed";
        break;
    case 2:
        typeName = "Distance";
    }
    emit getPlayersInfo(typeName, id, data);
    QBarSet* barSet = new QBarSet(typeName);
    QBarSeries* series = new QBarSeries;
    barSet->append(data);
    series->append(barSet);
    chart->addSeries(series);
    chart->setTitle(typeName);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    QStringList id_string;
    for (auto i: id) {
        id_string.append(QString::number(i));
    }
    axisX->append(id_string);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    QValueAxis *axisY = new QValueAxis();
    qreal data_max = *std::max_element(data.begin(), data.end());
    axisY->setRange(0,int(data_max*1.1));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    chart->legend()->setVisible(true);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->scrollAreaWidgetContents->setMinimumWidth(id.length()*30);
    update();
}
