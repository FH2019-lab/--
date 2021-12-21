#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qdebug.h>
#include <QtWidgets>
#include <QtWidgets>
#include <QVideoWidget>
#include <algorithm>
#include <QTextStream>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(this->width(), this->height());

    connect(ui->pushButton_load, &QAbstractButton::clicked, this, &MainWindow::loadVideo);
    //set slider, connect with video position
    connect(ui->horizontalSlider, &QAbstractSlider::sliderMoved, ui->video_label_left, &DisplayLabel::setVideoPostion);
    connect(ui->horizontalSlider, &QAbstractSlider::sliderMoved, ui->video_label_right, &DisplayLabel::setVideoPostion);
    connect(ui->video_label_left, &DisplayLabel::videoDurationChanged, this, &MainWindow::setVideoDuration);
    connect(ui->video_label_left, &DisplayLabel::videoPositionChanged, ui->horizontalSlider, &QAbstractSlider::setValue);
    ui->horizontalSlider->setDisabled(true);
    // 设置信息传递
    connect(ui->video_label_left, &DisplayLabel::sendVideoInfo, this, &MainWindow::setVideoInfo);
    //set play/pause button
    ui->pushButton_play->setEnabled(false);
    ui->pushButton_play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(ui->pushButton_play, &QAbstractButton::clicked, this, &MainWindow::playOrStop);
    //set timer
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, ui->video_label_left, &DisplayLabel::playNextFrame);
    connect(timer, &QTimer::timeout, ui->video_label_right, &DisplayLabel::playNextFrame);
    //set checkbox of boundingbox
    connect(ui->checkBox, &QCheckBox::stateChanged, ui->video_label_left, &DisplayLabel::showLabel);
    connect(ui->checkBox, &QCheckBox::stateChanged, ui->video_label_right, &DisplayLabel::showLabel);
    //disable label midification
    ui->checkBox_editLabel->setDisabled(true);
    ui->groupBox_editPenal->setDisabled(true);
    //set tableView and model
    itemModel_labels = new QStandardItemModel(0,0,this);
    ui->tableView_labels->setModel(itemModel_labels);
    ui->tableView_labels->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(itemModel_labels, &QAbstractItemModel::dataChanged, this, &MainWindow::change_label_id);
    // 处理视频中被选中的标记框发生变化
    connect(ui->video_label_left, &DisplayLabel::selectedLabelChanged, this, &MainWindow::selectedLabelChanged_left);
    connect(ui->video_label_right, &DisplayLabel::selectedLabelChanged, this, &MainWindow::selectedLabelChanged_right);
    // 处理id的变换情况，相应地变换combobox中的可选id;
    connect(ui->video_label_left, &DisplayLabel::idRemoved, this, &MainWindow::idRemoved);
    connect(ui->video_label_right, &DisplayLabel::idRemoved, this, &MainWindow::idRemoved);
    connect(ui->video_label_left, &DisplayLabel::idCreated, this, &MainWindow::idCreated);
    connect(ui->video_label_right, &DisplayLabel::idCreated, this, &MainWindow::idCreated);
    // 创建变量
    players.resize(60);
    // 设置basicinfo模块，设置球衣号输入框只能输入数字
    ui->groupBox_basic_info->setDisabled(true);
    ui->lineEdit_number->setValidator(new QIntValidator(0,99,this));
    // 设置按钮图标
    ui->pushButton_3->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    ui->pushButton_4->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    ui->pushButton_saveLabelFile->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    // 设置添加队伍窗口
    teamDialog = new AddTeamDialog();
    connect(teamDialog, &AddTeamDialog::newTeamAdded, this, &MainWindow::addTeam);
    // 设置标定足球场窗口
    calibrationDialog = new CalibrationDialog();
    connect(calibrationDialog, &CalibrationDialog::sendTransforms, this, &MainWindow::getTransforms);
    // 设置更新小图函数
    connect(ui->video_label_left, &DisplayLabel::boxPositionChanged, this, &MainWindow::playersPositionChanged_left);
    connect(ui->video_label_right, &DisplayLabel::boxPositionChanged, this, &MainWindow::playersPositionChanged_right);
    // 初始化绘图格式、字体
    pen_point.setColor(Qt::red);
    pen_point.setWidth(14);
    pen_number.setColor(Qt::black);
    pen_number.setWidth(5);
    font_number.setPointSize(16);
    font_number.setBold(true);
    font_number.setFamily("Microsoft YaHei");
    QColor color(Qt::red);
    for (int i=0; i<20; ++i){
        color.setAlpha(5*i);
        pen_trace[i].setColor(color);
        pen_trace[i].setWidth(14*20/(40-i));
    }
    // 初始化field2label
    this->show();
    field.load(":/pic/field.png");
    QRect image_rect(0, 0, field.width(), field.height());
    QRect label_rect(ui->picture_field->geometry().x(), ui->picture_field->geometry().y(), ui->picture_field->geometry().width(), ui->picture_field->geometry().height());
    // qDebug() << label_rect.x() << ' ' << label_rect.y() << ' ' << label_rect.width() << ' ' << label_rect.height();
    QTransform::quadToQuad(QPolygon(image_rect), QPolygon(label_rect), field2label);
    // 初始化折线图相关
    chart_distance = new QChart;
    chart_speed = new QChart;
    ui->chartView_distance->setChart(chart_distance);
    ui->chartView_speed->setChart(chart_speed);
    axis_y_speed = new QValueAxis;
    axis_y_speed->setTickCount(5);
    axis_y_speed->setTitleText("speed");
    chart_speed->addAxis(axis_y_speed, Qt::AlignLeft);
    axis_y_distance = new QValueAxis;
    axis_y_distance->setTickCount(5);
    axis_y_distance->setTitleText("distance");
    chart_distance->addAxis(axis_y_distance, Qt::AlignLeft);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadVideo()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a video file for left"), "", "(*.mp4 *.avi)");
    if (fileName.isEmpty()) return; //if the file name is empty, then return;
    // player_left->setSource(QUrl(fileName));
    QImage img = ui->video_label_left->LoadVideo(fileName);
    calibrationDialog->setImg(img, 0);
    calibrationDialog->exec();

    fileName = QFileDialog::getOpenFileName(this, tr("Choose a video file for right"), "", "*.mp4 *.avi");
    if (fileName.isEmpty()) return;
    img = ui->video_label_right->LoadVideo(fileName);
    calibrationDialog->setImg(img, 1);
    calibrationDialog->exec();

    ui->pushButton_play->setEnabled(true);
}

void MainWindow::playOrStop(){
    // qDebug()<<"timer state "<<timer->isActive();
    if(!timer->isActive())
        playVideo();
    else
        stopVideo();
}

void MainWindow::playVideo(){
    timer->start(33);
    ui->pushButton_play->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
}

void MainWindow::stopVideo(){
    timer->stop();
    ui->pushButton_play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void MainWindow::setVideoDuration(int duration){
    ui->horizontalSlider->setRange(0, duration);
}


void MainWindow::on_checkBox_editLabel_clicked(bool checked)
{
    // edit label选框被选中
    if(checked){
        stopVideo();
        ui->pushButton_play->setEnabled(false);
        ui->groupBox_editPenal->setEnabled(true);
        ui->checkBox->setChecked(true);
        getLabels();
        ui->video_label_left->setMode(1); //切到EDITING
        ui->video_label_right->setMode(1);
    }
    else{
        ui->pushButton_play->setEnabled(true);
        ui->groupBox_editPenal->setEnabled(false);
        itemModel_labels->clear();
        ui->video_label_right->setMode(0); //切回DISPLAY
        ui->video_label_right->setMode(0);
    }
}


void MainWindow::on_pushButton_loadlabels_clicked()
{
    /* 加载标签文件
     * 统计id信息，将id加入combobox
     */
    ui->comboBox_id->clear();
    ui->comboBox_id->addItem("<none>");
    for (int i=0; i<players.length(); ++i)
        players[i].valid=0;
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a label file for left video"), "", "*.txt");
    if (fileName.isEmpty()) return;
    QList<int> cnts = ui->video_label_left->LoadLabelFile(fileName);
    ui->label_filename_left->setText(fileName);
    for (int i=0; i<cnts.length(); ++i)
        if (cnts[i]>0)
            players[i].valid++;
    const QString fileName_r = QFileDialog::getOpenFileName(this, tr("Choose a label file for right video"), "", "*.txt");
    if (fileName_r.isEmpty()) return;
    cnts = ui->video_label_right->LoadLabelFile(fileName_r);
    ui->label_filename_right->setText(fileName_r);
    for (int i=0; i<cnts.length(); ++i)
        if (cnts[i]>0)
            players[i].valid++;
    for (int i=0; i<players.length(); ++i)
        if(players[i].valid>0)
            ui->comboBox_id->addItem(QString::number(i));
    ui->checkBox_editLabel->setEnabled(true);
}

void MainWindow::getLabels(){
    /*
     * 获取当前的标签数据
     * 输出到ui界面中的listwidget
     */
    itemModel_labels->clear();
    //恢复表头
    QStringList headers = {"id", "left/right"};
    itemModel_labels->setHorizontalHeaderLabels(headers);

    QList<box_t> labels = ui->video_label_left->getLabels();
    cnt_labels_left = labels.length();
    for (auto l:labels){
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(l.id)));
        QStandardItem* item = new QStandardItem("left");
        item -> setFlags(Qt::NoItemFlags); //设置只读
        row.append(item);
        itemModel_labels->appendRow(row);
    }
    labels = ui->video_label_right->getLabels();
    for (auto l:labels){
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(l.id)));
        QStandardItem* item = new QStandardItem("right");
        item -> setFlags(Qt::NoItemFlags);
        row.append(item);
        itemModel_labels->appendRow(row);
    }
}


void MainWindow::on_tableView_labels_clicked(const QModelIndex &index)
{
    //某个数据被选中
    // qDebug()<<index.column()<<' '<<index.row();
    if (itemModel_labels->item(index.row(), 1)->text()=="left"){
        ui->video_label_left->setSelectedLabel(index.row());
        ui->video_label_right->setSelectedLabel(-1);
    }
    else{
        ui->video_label_left->setSelectedLabel(-1);
        ui->video_label_right->setSelectedLabel(index.row()-cnt_labels_left);
    }
}



void MainWindow::on_pushButton_3_clicked()
{
    // 设置编辑面板中 previous 被点击
    ui->video_label_left->playPreviousFrame();
    ui->video_label_right->playPreviousFrame();
    getLabels();
}


void MainWindow::on_pushButton_4_clicked()
{
    // 设置编辑面板中 next 被点击
    ui->video_label_left->playNextFrame();
    ui->video_label_right->playNextFrame();
    getLabels();
}

void MainWindow::selectedLabelChanged_left(int idx){
    if (idx!=-1){
        ui->video_label_right->setSelectedLabel(-1);
        ui->tableView_labels->selectRow(idx);
    }
}

void MainWindow::selectedLabelChanged_right(int idx){
    if (idx!=-1){
        ui->video_label_left->setSelectedLabel(-1);
        ui->tableView_labels->selectRow(idx+cnt_labels_left);
    }
}

void MainWindow::on_pushButton_saveLabelFile_clicked()
{
    /*选择路径保存文件
     * 保存格式为：
     */
    QString fileName = QFileDialog::getSaveFileName(this, tr("save labels for left ..."), ui->label_filename_left->text(), "*.txt");
    QFile file(fileName);
    auto allLabels = ui->video_label_left->getAllLabels();
    int i=-1;
    if (file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
            QTextStream stream(&file);
            for (auto& labels:*allLabels) {
                i++;
                for(auto label:labels)
                    stream << i << ',' << label.id << ',' << label.rect.x() << ','
                           << label.rect.y() << ',' << label.rect.width() << ','
                           << label.rect.height() << ",0,-1,-1,-1" << Qt::endl;
            }
    }
}

void MainWindow::change_label_id(const QModelIndex idx, const QModelIndex, const QList<int>){
    // qDebug() << idx.row() << ' ' << idx.column();
    if (idx.row() < cnt_labels_left)
        ui->video_label_left->setID(idx.row(), itemModel_labels->item(idx.row(), idx.column())->text().toInt());
    else
        ui->video_label_right->setID(idx.row()-cnt_labels_left, itemModel_labels->item(idx.row(), idx.column())->text().toInt());
}

void MainWindow::idRemoved(int id){
    if (--players[id].valid <=0){
        int idx = ui->comboBox_id->findText(QString::number(id));
        if (idx!=-1)
            ui->comboBox_id->removeItem(idx);
    }
}

void MainWindow::idCreated(int id){
    if (players[id].valid++ == 0){
        ui->comboBox_id->addItem(QString::number(id));
    }
}

void MainWindow::on_lineEdit_name_textEdited(const QString &arg1)
{
    players[ui->comboBox_id->currentText().toInt()].name = arg1;
}



void MainWindow::on_lineEdit_number_textEdited(const QString &arg1)
{
    players[ui->comboBox_id->currentText().toInt()].number = arg1.toInt();
}


void MainWindow::on_comboBox_id_currentTextChanged(const QString &arg1)
{
    // 更新basic information中的内容
    if (arg1=="<none>")
        ui->groupBox_basic_info->setDisabled(true);
    else{
        ui->groupBox_basic_info->setEnabled(true);
        int id = arg1.toInt();
        ui->lineEdit_name->setText(players[id].name);
        if (players[id].number!=-1)
            ui->lineEdit_number->setText(QString::number(players[id].number));
        else
            ui->lineEdit_number->clear();
        int team_idx = ui->comboBox_team->findText(players[id].team);
        if (team_idx == -1)
            ui->comboBox_team->setCurrentIndex(0);
        else
            ui->comboBox_team->setCurrentIndex(team_idx);
    }
    updateChart();
    update();
}


void MainWindow::on_comboBox_team_currentTextChanged(const QString &arg1)
{
    if (arg1=="<none>")
        return;
    if (arg1=="<add new>"){
        teamDialog->exec();
    }
    else{
        players[ui->comboBox_id->currentText().toInt()].team = arg1;
    }
}

void MainWindow::addTeam(const QString &team){
    ui->comboBox_team->addItem(team);
    int idx = ui->comboBox_team->count()-1;
    ui->comboBox_team->setCurrentIndex(idx);
    players[ui->comboBox_id->currentText().toInt()].team = team;
}

void MainWindow::getTransforms(QTransform video2png, QTransform video2standard, int pos){
    if (pos == 0){
        video2png_left = video2png;
        video2standard_left = video2standard;
    }
    else{
        video2png_right = video2png;
        video2standard_right = video2standard;
    }
}

void MainWindow::playersPositionChanged_left(QList<QPoint> positions, QList<int> ids){
    // 这个函数与下面一个right的对称，记得同步修改！！！！！
    // 传递参数留给paintevevt处理
    positions_left = positions;
    ids_left = ids;

    // 添加球员历史信息
    QDateTime time = ui->video_label_left->video_time;
    int time_ms = time.toMSecsSinceEpoch();
    for (int i=0; i<positions.length(); ++i) {

        player_t* player = &players[ids[i]];
        if (player->appearedTimes.contains(time))
            continue;;
        int cnt = player->appearedTimes.size();
        player->appearedTimes.append(time);
        QPointF pos = video2standard_left.map(QPointF(positions[i]));
        player->positions.append(pos);
        player->positions_label.append(video2png_left.map(positions[i]));
        if (cnt == 0){
            player->series_speed = new QLineSeries;
            player->series_distance = new QLineSeries;
            player->series_speed->append(time_ms, 0);
            player->series_distance->append(time_ms, 0);
            continue;
        }
        qreal distance, distance_old;
        if (cnt > 5)
            distance = QLineF(player->positions[cnt-5], pos).length()/5;
        else
            distance = QLineF(player->positions[0], pos).length()/cnt;
        distance_old = (player -> series_distance->at(cnt-1)).y();
        player->series_distance->append(time_ms, distance_old+distance);
        qreal speed = distance*frame_rate;
        player->series_speed->append(time_ms, speed);
        player->max_distance = distance_old+distance;
        if (speed > player->max_speed)
            player->max_speed = speed;
    }

    updateChart();
    update();
}

void MainWindow::playersPositionChanged_right(QList<QPoint> positions, QList<int> ids){
    // 这个函数与上面一个left的对称，记得同步修改！！！！！
    positions_right = positions;
    ids_right = ids;

    // 添加球员历史信息
    QDateTime time = ui->video_label_right->video_time;
    int time_ms = time.toMSecsSinceEpoch();
    // qDebug() << "right " << time_ms;
    for (int i=0; i<positions.length(); ++i) {
        player_t* player = &players[ids[i]];
        if (player->appearedTimes.contains(time))
            continue;
        int cnt = player->appearedTimes.size();
        player->appearedTimes.append(time);
        QPointF pos = video2standard_right.map(QPointF(positions[i]));
        player->positions.append(pos);
        player->positions_label.append(video2png_right.map(positions[i]));
        if (cnt == 0){
            player->series_speed = new QLineSeries;
            player->series_distance = new QLineSeries;
            player->series_speed->append(time_ms, 0);
            player->series_distance->append(time_ms, 0);
            continue;
        }
        qreal distance, distance_old;
        if (cnt > 5)
            distance = QLineF(player->positions[cnt-5], pos).length()/5;
        else
            distance = QLineF(player->positions[0], pos).length()/cnt;
        distance_old = (player -> series_distance->at(cnt-1)).y();
        player->series_distance->append(time_ms, distance_old+distance);
        qreal speed = distance*frame_rate;
        player->series_speed->append(time_ms, speed);
        player->max_distance = distance_old+distance;
        if (speed > player->max_speed)
            player->max_speed = speed;
    }
    updateChart();
    update();
}



void MainWindow::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.setTransform(field2label);

    // 更新缩略图
    painter.drawImage(QPoint(0,0), field);
    painter.setFont(font_number);
    int radius = 7;
    for (int i=0; i<positions_left.length(); ++i){
        QPoint pos = video2png_left.map(positions_left[i]);
        // QPoint pos_real = field2label.map(pos);
        // qDebug() << "origin pos:" << positions_left[i].x() << ' ' << positions_right[i].y() << "position in field:" << pos.x() << pos.y()
        //         << " real position:" << pos_real.x() <<' ' << pos_real.y();
        painter.setPen(pen_point);
        painter.drawEllipse(pos, radius, radius);
        painter.setPen(pen_number);
        painter.drawText(QPoint(pos.x()-8, pos.y()+8), QString::number(ids_left[i]));
    }
    for (int i=0; i<positions_right.length(); ++i){
        QPoint pos = video2png_right.map(positions_right[i]);
        painter.setPen(pen_point);
        painter.drawEllipse(pos, radius, radius);
        painter.setPen(pen_number);
        painter.drawText(QPoint(pos.x()-8, pos.y()+8), QString::number(ids_right[i]));
    }
    //画轨迹图
    if (ui->comboBox_id->currentText()=="<none>" || ui->comboBox_id->currentText()=="")
        return;
    int id = ui->comboBox_id->currentText().toInt();
    player_t *player = &players[id];
    int time_now = ui->horizontalSlider->value()*1000/30;
    for (int i=0; i<player->appearedTimes.length(); ++i){
        int idx = player->appearedTimes[i].toMSecsSinceEpoch()*20/time_now;
        if (idx==20)
            break;
        painter.setPen(pen_trace[idx]);
        int r = pen_trace[idx].width()/2;
        painter.drawEllipse(player->positions_label[i], r, r);
    }
}


void MainWindow::updateChart(){
    if (ui->tabWidget->currentIndex()!=0)
        return;

    if (ui->comboBox_id->currentText()=="<none>" || ui->comboBox_id->currentText()=="")
        return;
    int id = ui->comboBox_id->currentText().toInt();
    // 更新图标上方文件显示
    int len = players[id].positions.length();
    if (len==0)
        return;
    ui->text_speed->setText("speed: "+QString::number(players[id].series_speed->at(len-1).y()));
    ui->text_distance->setText("distance travelled: "+QString::number(players[id].series_distance->at(len-1).y()));
    // 更新图表
    if (ui->tabWidget_chart->currentIndex() == 0){ //speed
        auto series = chart_speed->series();
        for (auto s : series){
            s->detachAxis(axis_x_speed);
            s->detachAxis(axis_y_speed);
            chart_speed->removeSeries(s);
        }
        if (ui->comboBox_id->currentText() == "<none>")
            return;
        chart_speed->addSeries(players[id].series_speed);
        players[id].series_speed->attachAxis(axis_x_speed);
        axis_y_speed->setMax(players[id].max_speed*1.1);
        players[id].series_speed->attachAxis(axis_y_speed);
        chart_speed->legend()->setVisible(false);
    }
    else{
        auto series = chart_distance->series();
        for (auto s : series){
            s->detachAxis(axis_x_distance);
            s->detachAxis(axis_y_distance);
            chart_distance->removeSeries(s);
        }
        if (ui->comboBox_id->currentText() == "<none>")
            return;
        chart_distance->addSeries(players[id].series_distance);
        players[id].series_distance->attachAxis(axis_x_distance);
        axis_y_distance->setMax(players[id].max_distance*1.1);
        players[id].series_distance->attachAxis(axis_y_distance);
        chart_distance->legend()->setVisible(false);
    }
}

void MainWindow::setVideoInfo(int video_frame_rate, QDateTime video_duration){
    // 设置图表坐标轴，及帧率
    frame_rate = video_frame_rate;
    axis_x_speed = new QDateTimeAxis;
    QDateTime time_start = QDateTime::fromMSecsSinceEpoch(0);
    axis_x_speed->setMin(time_start);
    axis_x_speed->setMax(video_duration);
    axis_x_speed->setFormat("m:ss");
    axis_x_speed->setTitleText("time");
    axis_x_speed->setTickCount(5);
    chart_speed->addAxis(axis_x_speed, Qt::AlignBottom);

    axis_x_distance = new QDateTimeAxis;
    axis_x_distance->setMin(time_start);
    axis_x_distance->setMax(video_duration);
    axis_x_distance->setFormat("m:ss");
    axis_x_distance->setTitleText("time");
    axis_x_distance->setTickCount(5);
    chart_distance->addAxis(axis_x_distance, Qt::AlignBottom);
    update();
}
