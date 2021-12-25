#ifndef DISPLAYLABEL_H
#define DISPLAYLABEL_H

#include <QLabel>
#include <QFile>
#include <QPainter>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <QMouseEvent>
#include <QDateTime>
#include <QTime>

struct box_t{
    int id=-1, cls = -1;
    QRectF rect;
    int x=-1, y=-1, z=-1;
    int selectedPoint=-1; //被选中的点，-1代表没有选中，0,1,2,3分别为左上，右上，右下，左下
};

class DisplayLabel : public QLabel
{
Q_OBJECT

public:
    explicit DisplayLabel(QWidget *parent = nullptr);
    QList<int> LoadLabelFile(QString, QList<int>&);
    QImage LoadVideo(QString);
    QList<box_t> getLabels();
    QList<QList<box_t>>* getAllLabels();
    void removeLabel(int);
    void setSelectedLabel(int);
    void getSelectedItem(QPoint, int&, int&);
    QImage getImage();
    QImage getPortrait(int idx);

    QDateTime video_time;
    int multiple = 2, offset=1;//一共几个这样的窗口


protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);


signals:
    void videoPositionChanged(int &);
    void videoDurationChanged(int &);
    void videoStopped();
    void selectedLabelChanged(int);
    void addingDone(); //用于告知主窗口已经完成了adding，取消另一个窗口的adding
    void idRemoved(int &); //某个id不复存在
    void idCreated(int &); //某个新的id出现
    void boxPositionChanged(QList<QPoint>, QList<int>); // 标签的位置发生变化，返回各个标注框底边的中点,视频坐标系
    void sendVideoInfo(int, QDateTime);

public slots:
    void playNextFrame();
    void playPreviousFrame();
    void setVideoPostion(int);
    void showLabel(int);
    void setMode(int);
    void setID(int idx, int val);

private:
    QFile file;  //存储标注的文件
    QList<QList<box_t>> labels_list;
    cv::VideoCapture video;  //视频源
    cv::Mat videoFrame;  //当前播放帧
    QDateTime video_duration;
    int frameidx;  //当前帧标号
    int video_frame_rate;
    QTransform frame2label; //从图像坐标到label中坐标的变化
    QTransform label2frame;
    bool isShowLabel=false; //是否显示标注
    enum mode_t{
        DISPLAY,
        EDITING,
        ADDING,
    }mode = DISPLAY;
    enum editMode_t{
        NONE,
        RESIZING,
        MOVING,
    }editMode=NONE;
    QPoint last_pos; //临时存储移动前的位置、添加时第一个点的位置
    bool mousePressed=false;
    bool alreadyAdd1=false;
    int selected_label=-1; //被选中的标签
    QList<int> id_cnts; // 存储每个id各出现了几次

    QPen pen_box;
    QPen pen_box_chosen;
    QPen pen_points;
    QPen pen_text;
    QFont font_text;
};

#endif // DISPLAYLABEL_H
