#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <QPainter>
#include <QMouseEvent>

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    void setImg(QImage img, int pos);
    ~CalibrationDialog();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void accept();

signals:
    void sendTransforms(QTransform video2png, QTransform video2standard, int pos);

private:
    int getSelectedPoint(QPoint);

    Ui::CalibrationDialog *ui;
    QTransform img2label;
    QTransform field2label;
    QTransform label2img;
    QImage field_outline;
    QImage image;
    QPolygon field_rect; //png中的场地
    QPolygon field_polygon;
    QPen pen_box;
    QPen pen_point;
    bool holding_point=false; //是否正在拖动点
    int selected_point=-1;
    int half; //0代表左半边，1代表右半边

    int field_horizon_padding = 53;
    int field_vertical_padding = 21;
};

#endif // CALIBRATIONDIALOG_H
