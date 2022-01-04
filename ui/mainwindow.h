#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QWidget>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <QTimer>
#include <QStandardItemModel>
#include <QList>
#include "displayLabel.h"
#include "addteamdialog.h"
#include "calibrationdialog.h"
#include <QtCharts>
#include <QDateTimeAxis>
#include <QLineF>
#include "mergedialog.h"
#include "barchartdialog.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QAbstractButton;
class QSlider;
class QLabel;
class QUrl;
QT_END_NAMESPACE

struct player_t{
    int valid=0;
    QString name="";
    QString team="";
    int number=-1;
    QImage pic;
    bool is_football=false;
    DisplayLabel *lastAppeared_window;
    QPoint lastAppeared_position; //视频坐标系
    int lastAppeared_time_ms, firstAppeared_time_ms;
    QList<QPointF> positions; //标准足球场
    QList<QPointF> positions_label; //png
    QList<QDateTime> appearedTimes;
    QLineSeries *series_speed=NULL, *series_distance=NULL;
    qreal max_speed=0, max_distance=0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void loadVideo();
    void playVideo();
    void stopVideo();
    void setVideoDuration(int);
    void playOrStop();
    void on_checkBox_editLabel_clicked(bool checked);
    void on_pushButton_loadlabels_clicked();
    void on_tableView_labels_clicked(const QModelIndex &index);
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void selectedLabelChanged_left(int);
    void selectedLabelChanged_right(int);
    void on_pushButton_saveLabelFile_clicked();
    void change_label_id(const QModelIndex, const QModelIndex, const QList<int>);
    void idRemoved(int);
    void idCreated(int);
    void on_lineEdit_name_textEdited(const QString &arg1);
    void on_lineEdit_number_textEdited(const QString &arg1);
    void on_comboBox_id_currentTextChanged(const QString &arg1);
    void on_comboBox_team_currentTextChanged(const QString &arg1);
    void addTeam(const QString &team);
    void getTransforms(QTransform video2png, QTransform video2standard, int pos);
    void playersPositionChanged_left(QList<QPoint>, QList<int>);
    void playersPositionChanged_right(QList<QPoint>, QList<int>);
    void setVideoInfo(int, QDateTime);
    void addingDone();
    void on_pushButton_addNewLabel_clicked();
    void on_pushButton_generatelabel_clicked();
    void on_pushButton_merge_clicked();
    void getPlayerInfo(int idx, QString &name, QString &team, QString &number, QImage &pic);
    void getPlayersInfo(QString, QList<int>&, QList<qreal>&);
    void on_pushButton_comparation_clicked();

    void on_comboBox_detail_currentIndexChanged(int index);

private:
    void getLabels();
    void updateChart();
    void updateHeat();
    int mergeId(int, int);

    Ui::MainWindow *ui;
    QTimer *timer;
    QStandardItemModel *itemModel_labels;
    int cnt_labels_left; //左侧视频的标签数量，用于计算右侧的标签对应的是右侧的第几个
    QList<player_t> players;

    AddTeamDialog* teamDialog;
    CalibrationDialog* calibrationDialog;
    MergeDialog* mergeDialog;
    BarChartDialog* barChartDialog;

    QImage image_field, image_outline;
    QTransform video2png_left, video2png_right, video2standard_left, video2standard_right;
    QTransform field2label, field2heat; //png坐标到展示坐标（mainwindow yuan点）的转换,png到热力图转化(以heat左上为0点）
    QList<QPoint> positions_left, positions_right;
    QList<int> ids_left, ids_right;

    QPen pen_point;
    QPen pen_point_ball;
    QPen pen_number;
    QPen pen_trace[20];
    QFont font_number;
    QColor colors_heat[256];
    QImage dataImg, heatImg;
    QList<int> count_positions;
    int maxCount=0;
    int heatMapWidth = 1;
    //QPainter* painter;

    QChart *chart_speed, *chart_distance;
    QDateTimeAxis* axis_x_speed, *axis_x_distance;
    QValueAxis *axis_y_speed, *axis_y_distance;
    int frame_rate;

};
#endif // MAINWINDOW_H
