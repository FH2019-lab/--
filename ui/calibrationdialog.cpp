#include "calibrationdialog.h"
#include "ui_calibrationdialog.h"
#include <qdebug.h>

CalibrationDialog::CalibrationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);
    setFixedSize(this->width(), this->height());
    field_outline.load(":/pic/field_outline.png");
    // qDebug() << field_outline.width() << ' ' << field_outline.height();
    field_polygon = QPolygon(QRect(20,10, ui->label->geometry().width()-40, ui->label->geometry().height()-20));
    field_rect = QPolygon(QRect(field_horizon_padding, field_vertical_padding, field_outline.width()-106, field_outline.height()-42));
    QTransform::quadToQuad(field_rect, field_polygon, field2label);

    pen_box.setWidth(2);
    pen_box.setColor(Qt::green);
    pen_point.setWidth(2);
    pen_point.setColor(Qt::green);
    // 跟踪鼠标
    this->setMouseTracking(true);
    ui->label->setMouseTracking(true);
}

CalibrationDialog::~CalibrationDialog()
{
    delete ui;
}

void CalibrationDialog::setImg(QImage img, int pos){
    /*设置要标定的视频图像，
     * pos可以为0/1，0是左边,1是右边
     */
    image = img;
    half = pos;
    int label_x = ui->label->geometry().x();
    int label_y = ui->label->geometry().y();
    int label_width = ui->label->geometry().width();
    int label_height = ui->label->geometry().height();
    int img_width = img.width();
    int img_height = img.height();
    double scale = std::min(label_height*1.0/img_height, label_width*0.5/img_width);
    // qDebug() << label_height << ' ' << img_height << ' ' << label_width << ' ' << img_width << ' ' << scale;
    QPolygonF canvas_polygon, img_polygon;
    double label_x0 = label_x + (label_width*0.5 - img_width*scale)/2;
    if (pos==1)
        label_x0+=label_width*0.5;
    double label_x1 = label_x0 + img_width*scale;
    double label_y0 = label_y + (label_height - img_height*scale)/2;
    double label_y1 = label_y0 + img_height*scale;
    // qDebug() << "x0 " << label_x0 << " x1 " << label_x1 << " y0 " << label_y0;
    canvas_polygon << QPointF(label_x0, label_y0) << QPointF(label_x0, label_y1)
                   << QPointF(label_x1, label_y1) << QPointF(label_x1, label_y0);
    img_polygon << QPointF(0., 0.) << QPointF(0., (double)img_height)
                << QPointF((double)img_width, (double)img_height) << QPointF((double)img_width, 0.);
    QTransform::quadToQuad(img_polygon, canvas_polygon, img2label);
    QTransform::quadToQuad(canvas_polygon, img_polygon, label2img);
    // todo:添加自动识别函数
    update();
}

void CalibrationDialog::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.setTransform(img2label);
    painter.drawImage(QPoint(0,0), image);
    painter.setTransform(field2label);
    painter.drawImage(QPoint(0,0), field_outline);

    painter.resetTransform();
    painter.setPen(pen_box);
    painter.drawPolygon(field_polygon);
    painter.setPen(pen_point);
    for (int i=0; i<4; ++i)
        painter.drawRect(field_polygon.point(i).x()-1, field_polygon.point(i).y()-1, 2, 2);
}


void CalibrationDialog::mousePressEvent(QMouseEvent *event){
    QPoint pos = event->pos();
    selected_point = getSelectedPoint(pos);
}

void CalibrationDialog::mouseMoveEvent(QMouseEvent *event){
    //vqDebug() << "moving " << selected_point;
    QPoint pos = event->pos();
    if (selected_point == -1){
        int selected = getSelectedPoint(pos);
        // qDebug() << "selecting" << selected;
        if (selected == 0 || selected == 2)
            this->setCursor(Qt::SizeFDiagCursor);
        else if (selected == 1 || selected == 3)
            this->setCursor(Qt::SizeBDiagCursor);
        else
            this->setCursor(Qt::ArrowCursor);
    }
    else{
        field_polygon.setPoint(selected_point, pos);
        QTransform::quadToQuad(field_rect, field_polygon, field2label);
        update();
    }
}

void CalibrationDialog::mouseReleaseEvent(QMouseEvent *){
    selected_point = -1;
}

void CalibrationDialog::accept(){
    QPolygonF field_img = label2img.map(QPolygonF(field_polygon)); //场地四边形映射到视频图片中
    QPolygonF field_standard; //标准足球场，单位是米
    field_standard << QPoint(0., 0.) << QPoint(0., 68.) << QPoint(105., 68.) << QPoint(105., 0.);
    QTransform video2png, video2standard; // 从视频图片位置到标准图片的变换，从视频图片到标准足球场的变换
    QTransform::quadToQuad(field_img, field_rect, video2png);
    QTransform::quadToQuad(field_img, field_standard, video2standard);
    emit sendTransforms(video2png, video2standard, half);
    QDialog::accept();
}

int CalibrationDialog::getSelectedPoint(QPoint pos){
    for (int i=0; i<4; ++i)
        if ((pos-field_polygon.point(i)).manhattanLength() < 4)
            return i;
    return -1;
}
