#include "displayLabel.h"
#include<QString>
#include <qdebug.h>

DisplayLabel::DisplayLabel(QWidget *){
    // 初始化各个画笔
    pen_box.setWidth(4);
    pen_box.setColor(Qt::red);
    pen_box_chosen.setWidth(2);
    pen_box_chosen.setColor(Qt::green);
    pen_points.setWidth(8);
    pen_points.setColor(Qt::green);
    font_text.setPointSize(16);
    font_text.setBold(true);
    font_text.setFamily("Microsoft YaHei");
    // 跟踪鼠标
    this->setMouseTracking(true);
    // 初始化数组
    id_cnts.resize(30);
}

QList<int> DisplayLabel::LoadLabelFile(QString filename, QList<int> &footballs){
    // 从文件中读取标签信息
    // 每行格式：<frame>, <id>, <cls>, <bb_left>, <bb_top>, <bb_width>, <bb_height>, <conf>, <x>, <y>, <z>
    // 其中frame从1开始计数
    footballs.resize(id_cnts.length());
    for (int i=0; i< id_cnts.length(); i++) {
        id_cnts[i] = 0;
        footballs[i]=0;
    }
    if (filename.isEmpty())
        return id_cnts;
    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return id_cnts;
    if (!file.exists())
        return id_cnts;
    QTextStream in(&file);
    labels_list.clear();
    int frame_num = 0;
    QVector<box_t> labels;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList line_prt = line.split(',');
        if (line_prt.at(0).toInt() != frame_num){
            frame_num = line_prt.at(0).toInt();
            labels_list.append(labels);
            labels.clear();
        }
        box_t bbox;
        bbox.rect.setRect(line_prt.at(3).toDouble(), line_prt.at(4).toDouble(), line_prt.at(5).toDouble(), line_prt.at(6).toDouble());
        bbox.id = line_prt.at(1).toInt();
        bbox.cls = line_prt.at(2).toInt();
        if (bbox.id >= id_cnts.length()){
            id_cnts.resize(bbox.id + 10);
            footballs.resize(bbox.id+10);
        }
        if (bbox.id > -1)
            id_cnts[bbox.id]++;
        if (bbox.cls == 1)
            footballs[bbox.id]++;
        labels.append(bbox);
    }
    labels_list.append(labels);
    file.close();
    update();
    return id_cnts;
}

QImage DisplayLabel::LoadVideo(QString filename){
    //函数：加载视频并返回第一帧
    video = cv::VideoCapture(filename.toStdString());
    int duration = video.get(cv::CAP_PROP_FRAME_COUNT);
    emit videoDurationChanged(duration);

    //设置视频时常
    video_frame_rate = video.get(cv::CAP_PROP_FPS);
    video_duration = QDateTime::fromMSecsSinceEpoch(int(duration*1000/video_frame_rate));
    emit sendVideoInfo(video_frame_rate, video_duration);

    // 根据视频画面大小设置视频到窗口的变化矩阵frame2label
    int frame_width = video.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = video.get(cv::CAP_PROP_FRAME_HEIGHT);
    int label_width = QLabel::geometry().width();
    int label_height = QLabel::geometry().height();
    QPolygonF frame_polygen, label_polygen;
    frame_polygen << QPointF(0., 0.) << QPointF(0., (double)frame_height)
                  << QPointF((double)frame_width, (double)frame_height) << QPointF((double)frame_width, 0.);
    double scale = std::min(label_width*1.0/frame_width, label_height*1.0/frame_height); //缩放比例
    double x0 = (label_width - frame_width*scale)/2;
    double x1 = x0 + frame_width*scale;
    double y0 = (label_height - frame_height*scale) / 2;
    double y1 = y0 + frame_height*scale;
    label_polygen << QPointF(x0, y0) << QPointF(x0, y1) << QPointF(x1, y1) << QPointF(x1, y0);
    QTransform::quadToQuad(frame_polygen, label_polygen, frame2label);
    QTransform::quadToQuad(label_polygen, frame_polygen, label2frame);
    playNextFrame();
    QImage img = QImage((const uchar*)videoFrame.data, videoFrame.cols, videoFrame.rows, videoFrame.step, QImage::Format_RGB888);
    return img;
}

QList<box_t> DisplayLabel::getLabels(){
    return labels_list[frameidx];
}

QList<QList<box_t>>* DisplayLabel::getAllLabels(){
    return &labels_list;
}

void DisplayLabel::removeLabel(int idx){
    // 删除第idx个标签
    labels_list[frameidx].erase(labels_list[frameidx].cbegin()+idx);
}

void DisplayLabel::setSelectedLabel(int idx){
    selected_label = idx;
    update();
}

void DisplayLabel::getSelectedItem(QPoint mousePos, int &s_label, int &s_point){
    // 根据鼠标位置计算哪个标注框/点被选中了
    s_label = -1;
    s_point = -1;
    auto bbox = labels_list[frameidx].begin();
    for (int i=0; i<labels_list[frameidx].length(); ++i) {
        if ((frame2label.map(bbox->rect.topLeft())-mousePos).manhattanLength()<6){
            s_label = i;
            s_point = 0;
            break;
        }
        if ((frame2label.map(bbox->rect.topRight())-mousePos).manhattanLength()<6){
            s_label = i;
            s_point = 1;
            break;
        }
        if ((frame2label.map(bbox->rect.bottomRight())-mousePos).manhattanLength()<6){
            s_label = i;
            s_point = 2;
            break;
        }
        if ((frame2label.map(bbox->rect.bottomLeft())-mousePos).manhattanLength()<6){
            s_label = i;
            s_point = 3;
            break;
        }
        //todo::选面积小的
        if (s_label==-1 && frame2label.mapRect(bbox->rect).contains(mousePos))
            s_label = i;
        bbox++;
    }
}

QImage DisplayLabel::getImage(){
    QImage img = QImage((const uchar*)videoFrame.data, videoFrame.cols, videoFrame.rows, videoFrame.step, QImage::Format_RGB888);
    return img;
}

void DisplayLabel::setVideoPostion(int pos){
    frameidx = pos;
    video.set(cv::CAP_PROP_POS_FRAMES, pos);
}

void DisplayLabel::showLabel(int val){
    if (val==0)
        isShowLabel = false;
    else
        isShowLabel = true;
    update();
}

void DisplayLabel::setMode(int m){
    switch(m){
    case 0:
        mode = DISPLAY;
        break;
    case 1:
        mode = EDITING;
        break;
    case 2:
        mode = ADDING;
    }
}

void DisplayLabel::setID(int idx, int val){
    int val_old = labels_list[frameidx][idx].id;
    if(val_old>-1 && --id_cnts[val_old] == 0)
        emit idRemoved(val_old);
    labels_list[frameidx][idx].id = val;
    //todo::size out of range
    if(val>-1 && id_cnts[val]++==0)
        emit idCreated(val);
}

void DisplayLabel::playNextFrame(){
    video>>videoFrame;
    if (!videoFrame.data){
        emit videoStopped();
        return;
    }
    cv::cvtColor(videoFrame, videoFrame, cv::COLOR_BGR2RGB);
    frameidx = video.get(cv::CAP_PROP_POS_FRAMES);
    video_time = QDateTime::fromMSecsSinceEpoch(int(frameidx*1000/video_frame_rate));
    emit videoPositionChanged(frameidx);
    selected_label = -1;
    update();

    // 返回位置序列
    QList<QPoint> positions;
    QList<int> ids;
    if (labels_list.length()==0)
        return;
    for (auto bbox: labels_list[frameidx]) {
        positions.append(QPoint(bbox.rect.x()+bbox.rect.width()*0.5, bbox.rect.y()+bbox.rect.height()));
        ids.append(bbox.id);
    }
    emit boxPositionChanged(positions, ids);
}

void DisplayLabel::playPreviousFrame(){
    video.set(cv::CAP_PROP_POS_FRAMES, frameidx-2);
    playNextFrame();
}

void DisplayLabel::paintEvent(QPaintEvent *){
    if (!videoFrame.data){
        emit videoStopped();
        return;
    }
    //绘制视频帧
    QPainter painter(this);
    painter.setTransform(frame2label);
    painter.setFont(font_text);
    QImage img = QImage((const uchar*)videoFrame.data, videoFrame.cols, videoFrame.rows, videoFrame.step, QImage::Format_RGB888);
    painter.drawImage(QPoint(0,0), img);

    if (!isShowLabel)
        return; // 如果不展示标注框，返回
    //画标注框
    if (labels_list.length() < frameidx){
        //todo::warning
        return;
    }
    auto bbox = labels_list[frameidx].begin();
    for (int i=0; i<labels_list[frameidx].length(); ++i) {
        if (i == selected_label){
            // 画控制点
            painter.setPen(pen_points);
            painter.drawPoints(bbox->rect);
            painter.setPen(pen_box_chosen);
        }
        else
            painter.setPen(pen_box);
        painter.drawRect(bbox->rect);
        painter.drawText(bbox->rect.left()+8, bbox->rect.top()-8, QString::number(bbox->id*multiple+offset));
        bbox++;
    }

    if (mode == ADDING && !alreadyAdd1){
        //有单独点
        painter.setPen(pen_points);
        painter.drawPoint(label2frame.map(last_pos));
    }
}

void DisplayLabel::mousePressEvent(QMouseEvent *event){
    mousePressed = true;
    if (mode==DISPLAY)
        return;
    QPoint pos = event->pos();

    if (mode == ADDING){
        if (!alreadyAdd1){
            last_pos = pos;
            selected_label = -1;
        }
        else{
            //添加新框
            QRectF rect(label2frame.map(last_pos), label2frame.map(pos));
            box_t bbox;
            bbox.rect = rect;
            selected_label = labels_list[frameidx].length();
            labels_list[frameidx].append(bbox);
        }
    }
    else{ //EDITING
        int selected_point;
        getSelectedItem(pos, selected_label, selected_point);
        if (selected_label!=-1)
            labels_list[frameidx][selected_label].selectedPoint = selected_point;
        if (selected_point!=-1)
            editMode=RESIZING;
        else if (selected_label!=-1){
            editMode=MOVING;
            last_pos = pos;
        }
    }
    update();
    emit selectedLabelChanged(selected_label);
}

void DisplayLabel::mouseMoveEvent(QMouseEvent *event){
    if (mode==DISPLAY)
        return;
    QPoint pos = event->pos();
    auto pos_frame = label2frame.map(pos); //对应在图像中的坐标
    auto diff = pos_frame - label2frame.map(last_pos);
    int s_label=-1, s_point=-1;
    auto bbox = labels_list[frameidx].begin()+selected_label;

    if (mode == EDITING){
        switch (editMode) {
        case NONE: //没有正在改变选择框形态，在鼠标落入选框时改变样式
            getSelectedItem(pos, s_label, s_point);
            switch(s_point){
            case 0:case 2:
                this->setCursor(Qt::SizeFDiagCursor);
                break;
            case 1:case 3:
                this->setCursor(Qt::SizeBDiagCursor);
                break;
            default:
                if (s_label!=-1)
                    this->setCursor(Qt::SizeAllCursor);
                else
                    this->setCursor(Qt::ArrowCursor);
            }
            break;
        case RESIZING:
            switch (bbox->selectedPoint) {
            case 0:
                bbox->rect.setTopLeft(pos_frame);
                break;
            case 1:
                bbox->rect.setTopRight(pos_frame);
                break;
            case 2:
                bbox->rect.setBottomRight(pos_frame);
                break;
            case 3:
                bbox->rect.setBottomLeft(pos_frame);
            }
            update();
            break;
        case MOVING:
            bbox->rect.moveTo(bbox->rect.x()+diff.x(), bbox->rect.y()+diff.y());
            last_pos = pos;
            update();
        }
    }
    else{
        //ADDING
        if (!alreadyAdd1 || !mousePressed)
            return;
        labels_list[frameidx][selected_label].rect.setBottomRight(pos_frame);
        update();
    }
}

void DisplayLabel::mouseReleaseEvent(QMouseEvent *){
    mousePressed = false;
    switch (mode) {
    case ADDING:
        if (alreadyAdd1){
            mode = EDITING;
            emit addingDone();
        }
        alreadyAdd1 = !alreadyAdd1;
        break;
    case EDITING:
        editMode = NONE;
        if (selected_label != -1)
            labels_list[frameidx][selected_label].selectedPoint = -1;
        break;
    default:
        break;
    }

}

QImage DisplayLabel::getPortrait(int idx){
    QRectF rect;
    for (auto b: labels_list[frameidx]) {
        if (b.id == idx)
            rect = b.rect;
    }
    QImage img_frame = QImage((const uchar*)videoFrame.data, videoFrame.cols, videoFrame.rows, videoFrame.step, QImage::Format_RGB888);
    QImage img = img_frame.copy(int(rect.x()), int(rect.y()), int(rect.width()), int(rect.height()));
    return img;
}
