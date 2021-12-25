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

    //处理视频播放窗口
    ui->video_label_left->offset = 0;
    ui->video_label_right->offset = 1;
    // 处理视频中被选中的标记框发生变化
    connect(ui->video_label_left, &DisplayLabel::selectedLabelChanged, this, &MainWindow::selectedLabelChanged_left);
    connect(ui->video_label_right, &DisplayLabel::selectedLabelChanged, this, &MainWindow::selectedLabelChanged_right);
    // 处理add模式
    connect(ui->video_label_left, &DisplayLabel::addingDone, this, &MainWindow::addingDone);
    connect(ui->video_label_right, &DisplayLabel::addingDone, this, &MainWindow::addingDone);
    // 处理id的变换情况，相应地变换combobox中的可选id;
    connect(ui->video_label_left, &DisplayLabel::idRemoved, this, &MainWindow::idRemoved);
    connect(ui->video_label_right, &DisplayLabel::idRemoved, this, &MainWindow::idRemoved);
    connect(ui->video_label_left, &DisplayLabel::idCreated, this, &MainWindow::idCreated);
    connect(ui->video_label_right, &DisplayLabel::idCreated, this, &MainWindow::idCreated);

    // 创建变量
    players.resize(30);
    // 设置basicinfo模块，设置球衣号输入框只能输入数字
    ui->groupBox_basic_info->setDisabled(true);
    ui->lineEdit_number->setValidator(new QIntValidator(0,99,this));
    // 设置按钮图标
    ui->pushButton_3->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    ui->pushButton_4->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    ui->pushButton_2->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->pushButton_merge->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
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
    QColor color(Qt::darkRed);
    for (int i=0; i<20; ++i){
        color.setAlpha(4*i);
        pen_trace[i].setColor(color);
        pen_trace[i].setWidth(14*20/(40-i));
    }
    //根据线性渐变色条得到颜色表
    QLinearGradient linear=QLinearGradient(QPoint(0,0),QPoint(255,0));
    linear.setColorAt(0, Qt::blue);
    linear.setColorAt(0.4, Qt::blue);
    linear.setColorAt(0.5, Qt::cyan);
    linear.setColorAt(0.6, Qt::green);
    linear.setColorAt(0.8, Qt::yellow);
    linear.setColorAt(0.95, Qt::red);
    //把渐变色绘制到Img方便取颜色
    QImage img(256,1,QImage::Format_ARGB32);
    QPainter painter(&img);
    painter.fillRect(img.rect(),linear);
    //HeatAlpha为热力图整体透明度
    for(int i=0;i<256;i++){
        //根据热力图透明度来计算颜色表的透明度
        //颜色+透明度
        colors_heat[i] = img.pixel(i,0);
        colors_heat[i].setAlpha(100/255.0*i);
    }

    // 初始化field2label，field2display, 热力图相关
    this->show();
    image_field.load(":/pic/field.png");
    image_outline.load(":/pic/field_outline.png");
    QRect image_rect(0, 0, image_field.width(), image_field.height());
    // qDebug() << label_rect.x() << ' ' << label_rect.y() << ' ' << label_rect.width() << ' ' << label_rect.height();
    QTransform::quadToQuad(QPolygon(image_rect), QPolygon(ui->picture_field->geometry()), field2label);
    QRect rect_detail(0,0,ui->picture_detail->geometry().width(), ui->picture_detail->geometry().height());
    QTransform::quadToQuad(QPolygon(image_rect), QPolygon(rect_detail), field2heat);
    dataImg = QImage(rect_detail.width(), rect_detail.height(), QImage::Format_Alpha8);
    heatImg = QImage(rect_detail.width(), rect_detail.height(), QImage::Format_ARGB32);
    count_positions.resize(ui->picture_detail->geometry().height()*ui->picture_detail->geometry().width());
    for (int i=0; i<count_positions.length(); ++i)
        count_positions[i]=0;
    heatMapWidth = ui->picture_detail->geometry().width();

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

    //设置融合窗口
    mergeDialog = new MergeDialog;
    connect(mergeDialog, &MergeDialog::getInfo, this, &MainWindow::getPlayerInfo);

    // 设置画柱状图窗口
    barChartDialog = new BarChartDialog;
    connect(barChartDialog, &BarChartDialog::getPlayersInfo, this, &MainWindow::getPlayersInfo);
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
        if (cnts[i]>0){
            if (players.length()<=i*2)
                players.resize(i*2+10);
            players[i*2].valid++;
        }
    const QString fileName_r = QFileDialog::getOpenFileName(this, tr("Choose a label file for right video"), "", "*.txt");
    if (fileName_r.isEmpty()) return;
    cnts = ui->video_label_right->LoadLabelFile(fileName_r);
    ui->label_filename_right->setText(fileName_r);
    for (int i=0; i<cnts.length(); ++i)
        if (cnts[i]>0){
            if (players.length()<2*2+2)
                players.resize(i*2+10);
            players[i*2+1].valid++;
        }

    QStringList player_list;
    for (int i=0; i<players.length(); ++i)
        if(players[i].valid>0)
            player_list.append(QString::number(i));
    ui->comboBox_id->addItems(player_list);
    mergeDialog->setPlayerList(player_list);
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
    for (int i=0; i<ids.length(); ++i) {
        ids[i] = ids[i]*2;
    }
    ids_left = ids;

    //清空之前出现的信息
    QDateTime time = ui->video_label_left->video_time;
    int time_ms = time.toMSecsSinceEpoch();
    for (auto p : players) {
        if (p.lastAppeared_time_ms != time_ms)
            p.lastAppeared_window = NULL;
    }

    // 添加球员历史信息
    for (int i=0; i<positions.length(); ++i) {
        player_t* player = &players[ids[i]];
        QPoint pos_field = video2png_left.map(positions[i]);
        QPoint pos_detail = field2heat.map(pos_field);
        if (player->appearedTimes.contains(time))
            continue;
        int cnt = player->appearedTimes.size();
        player->lastAppeared_window = ui->video_label_left;
        player->lastAppeared_position = positions[i];
        int k = pos_detail.y()*heatMapWidth+pos_detail.x();
        if (k<0 || k>= count_positions.length()){
            qDebug() << "x" << pos_detail.x() << "y" << pos_detail.y();
        }
        else{
            if (++count_positions[k]>maxCount)
                maxCount=count_positions[k];
        }
        player->lastAppeared_time_ms = time_ms;
        player->appearedTimes.append(time);
        QPointF pos = video2standard_left.map(QPointF(positions[i]));
        player->positions.append(pos);
        player->positions_label.append(pos_field);
        if (cnt == 0){
            player->series_speed = new QLineSeries;
            player->series_distance = new QLineSeries;
            player->series_speed->append(time_ms, 0);
            player->series_distance->append(time_ms, 0);
            player->pic = ui->video_label_left->getPortrait(ids[i]/2);
            player->firstAppeared_time_ms = time_ms;
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
    updateHeat();
    update();
}

void MainWindow::playersPositionChanged_right(QList<QPoint> positions, QList<int> ids){
    // 这个函数与上面一个left的对称，记得同步修改！！！！！
    positions_right = positions;
    for (int i=0; i<ids.length(); ++i) {
        ids[i] = ids[i]*2+1;
    }
    ids_right = ids;

    //清空之前出现的信息
    QDateTime time = ui->video_label_left->video_time;
    int time_ms = time.toMSecsSinceEpoch();
    for (auto p : players) {
        if (p.lastAppeared_time_ms != time_ms)
            p.lastAppeared_window = NULL;
    }

    // 添加球员历史信息
    // qDebug() << "right " << time_ms;
    for (int i=0; i<positions.length(); ++i) {
        player_t* player = &players[ids[i]];
        QPoint pos_field = video2png_right.map(positions[i]);
        QPoint pos_detail = field2heat.map(pos_field);
        if (player->appearedTimes.contains(time))
            continue;
        int k = pos_detail.y()*heatMapWidth+pos_detail.x();
        if (k<0 || k>= count_positions.length())
            qDebug() << "x" << pos_detail.x() << "y" << pos_detail.y();
        else
            if (++count_positions[k]>maxCount)
                maxCount=count_positions[k];
        int cnt = player->appearedTimes.size();
        player->lastAppeared_window = ui->video_label_right;
        player->lastAppeared_position = positions[i];
        player->lastAppeared_time_ms = time_ms;
        player->appearedTimes.append(time);
        QPointF pos = video2standard_right.map(QPointF(positions[i]));
        player->positions.append(pos);
        player->positions_label.append(pos_field);
        if (cnt == 0){
            player->series_speed = new QLineSeries;
            player->series_distance = new QLineSeries;
            player->series_speed->append(time_ms, 0);
            player->series_distance->append(time_ms, 0);
            player->pic = ui->video_label_right->getPortrait(ids[i]/2);
            player->firstAppeared_time_ms = time_ms;
            continue;
        }
        qreal distance, distance_old;
        if (cnt > 20)
            distance = QLineF(player->positions[cnt-20], pos).length()/20;
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
    updateHeat();
    update();
}

void MainWindow::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.setTransform(field2label);

    // 更新缩略图
    painter.drawImage(QPoint(0,0), image_field);
    painter.setFont(font_number);


    //画轨迹图
    int id = ui->comboBox_id->currentText().toInt();
    player_t *player = &players[id];
    int time_now = ui->horizontalSlider->value()*1000/30;
    if (ui->comboBox_id->currentText()!="<none>" && ui->comboBox_id->currentText()!=""){
        for (int i=0; i<player->appearedTimes.length(); ++i){
            int idx = player->appearedTimes[i].toMSecsSinceEpoch()*20/time_now;
            if (idx==20)
                break;
            painter.setPen(pen_trace[idx]);
            int r = pen_trace[idx].width()/2;
            painter.drawEllipse(player->positions_label[i], r, r);
        }
    }

    //画点
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

    if (ui->comboBox_detail->currentText() == "detail:"){
    // 画细节放大图
        painter.resetTransform();
        if (player->lastAppeared_time_ms == time_now){
            if (player->lastAppeared_window == NULL)
                return;
            int box_width = ui->picture_detail->geometry().width();
            int box_height = ui->picture_detail->geometry().height();
            QImage cut = (player->lastAppeared_window->getImage()).copy(player->lastAppeared_position.x()-box_width/2,
                                                                      player->lastAppeared_position.y()-box_height*4/5,
                                                                      box_width, box_height);
            painter.drawImage(ui->picture_detail->geometry(), cut);
        }
    }
    else{
        //画热力图
        painter.resetTransform();
        painter.drawImage(ui->picture_detail->geometry(), image_outline);
        painter.drawImage(ui->picture_detail->geometry(), heatImg);
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
    ui->text_speed->setText("speed(instant): "+QString::number(players[id].series_speed->at(len-1).y(), 'f', 2)+"m/s");
    qreal distance = players[id].series_distance->at(len-1).y();
    ui->text_distance->setText("distance travelled: "+QString::number(distance) + "m");
    ui->text_speed_more->setText("speed(avg, max)"
                                 +QString::number(distance*1000./(players[id].lastAppeared_time_ms - players[id].firstAppeared_time_ms), 'f', 2)
                                 +","+QString::number(players[id].max_speed, 'f', 2));
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

void MainWindow::updateHeat(){
    //更新热力图
    if (ui->comboBox_detail->currentText()!="heatmap:")
        return;
    int time_now = ui->horizontalSlider->value()*1000/30;
    if (time_now%100==0){ //画慢点
        dataImg.fill(Qt::transparent);
        heatImg.fill(Qt::transparent);
        //draw dataImg
        QPainter painter2(&dataImg);
        painter2.setPen(Qt::transparent);
        int radius=5;
        for (int i=0; i<ui->picture_detail->geometry().height(); ++i){
            for(int j=0; j<heatMapWidth; ++j){
                int k=i*heatMapWidth+j;
                if(count_positions[k]==0)
                    continue;
                int alpha=count_positions[k]*255/maxCount;
                QRadialGradient gradient(j,i,radius);
                gradient.setColorAt(0,QColor(0,0,0,alpha));
                gradient.setColorAt(1,QColor(0,0,0,0));
                painter2.setBrush(gradient);
                painter2.drawEllipse(QPointF(j,i),radius,radius);
            }
        }

        //draw heatImg
        for(int row=0;row<ui->picture_detail->geometry().height();row++)
            for(int col=0;col<ui->picture_detail->geometry().width();col++)
                heatImg.setPixelColor(col, row, colors_heat[dataImg.pixelColor(col, row).alpha()]);
    }
}

int MainWindow::mergeId(int id1, int id2){
    // 融合到id2
    int id_new = (id1<id2)? id1:id2;
    if (id1%2 != id2%2)
        players[id_new].valid = 2;
    if (id_new == id1)
        players[id2].valid=0;
    else
        players[id1].valid=0;
    if (players[id1].name!="" && players[id2].name=="")
        players[id_new].name = players[id1].name;
    else
        players[id_new].name = players[id2].name;
    if (players[id1].team!="" && players[id2].team=="")
        players[id_new].team = players[id1].team;
    else
        players[id_new].team = players[id2].team;
    if (players[id1].number!=-1 && players[id2].number==-1)
        players[id_new].number = players[id1].number;
    else
        players[id_new].number = players[id2].number;

    if (players[id1].lastAppeared_time_ms > players[id2].lastAppeared_time_ms){
        players[id_new].lastAppeared_window = players[id1].lastAppeared_window;
        players[id_new].lastAppeared_position = players[id1].lastAppeared_position;
        players[id_new].lastAppeared_time_ms = players[id1].lastAppeared_time_ms;
    }
    else{
        players[id_new].lastAppeared_window = players[id2].lastAppeared_window;
        players[id_new].lastAppeared_position = players[id2].lastAppeared_position;
        players[id_new].lastAppeared_time_ms = players[id2].lastAppeared_time_ms;
    }
    players[id_new].max_distance = std::max(players[id1].max_distance, players[id2].max_distance);
    players[id_new].max_speed = std::max(players[id2].max_speed, players[id2].max_speed);


    QLineSeries *series_s1=players[id1].series_speed, *series_s2=players[id2].series_speed,
            *series_d1=players[id1].series_distance, *series_d2=players[id2].series_distance;
    QLineSeries *series_s_new, *series_d_new;
    series_s_new = new QLineSeries;
    series_d_new = new QLineSeries;
    QList<QPointF> positions, positions_label;
    QList<QDateTime> appeared_times;
    int i=0, j=0;
    while (i<players[id1].appearedTimes.length() && j<players[id2].appearedTimes.length()){
        qDebug() << "i" <<i << "j" << j;
        if (players[id1].appearedTimes[i] < players[id2].appearedTimes[j]){
            series_s_new->append(series_s1->at(i));
            series_d_new->append(series_d1->at(i));
            positions.append(players[id1].positions[i]);
            positions_label.append(players[id1].positions_label[i]);
            appeared_times.append(players[id1].appearedTimes[i]);
            i++;
        }
        else{
            series_s_new->append(series_s2->at(j));
            series_d_new->append(series_d2->at(j));
            positions.append(players[id2].positions[j]);
            positions_label.append(players[id2].positions_label[j]);
            appeared_times.append(players[id2].appearedTimes[j]);
            if (players[id1].appearedTimes[i] == players[id2].appearedTimes[j])
                i++;
            j++;
        }
    }
    while (i<players[id1].appearedTimes.length()){
        series_s_new->append(series_s1->at(i));
        series_d_new->append(series_d1->at(i));
        positions.append(players[id1].positions[i]);
        positions_label.append(players[id1].positions_label[i]);
        appeared_times.append(players[id1].appearedTimes[i]);
        i++;
    }
    while (j< players[id2].appearedTimes.length()){
        series_s_new->append(series_s2->at(j));
        series_d_new->append(series_d2->at(j));
        positions.append(players[id2].positions[j]);
        positions_label.append(players[id2].positions_label[j]);
        appeared_times.append(players[id2].appearedTimes[j]);
        j++;
    }
    players[id_new].positions = positions;
    players[id_new].positions_label = positions_label;
    players[id_new].appearedTimes = appeared_times;
    players[id_new].series_speed = series_s_new;
    players[id_new].series_distance = series_d_new;
    QLineSeries* series_list[4] = {series_s1, series_s2, series_d1, series_d2};
    for (auto s: series_list){
        if (s!=NULL){
            delete s;
            s=NULL;
        }
    }
    return id_new;
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

void MainWindow::addingDone(){
    ui->video_label_left->setMode(1); //切回EDITING
    ui->video_label_right->setMode(1);
    getLabels();
}


void MainWindow::on_pushButton_addNewLabel_clicked()
{
    ui->video_label_left->setMode(2); //切到ADDING
    ui->video_label_right->setMode(2);
}


void MainWindow::on_pushButton_generatelabel_clicked()
{

//    QProcess p(NULL);
//    p.setWorkingDirectory("D:/GoodGoodStudyDayDayUp/DaSanShang/DigitalImagePropressing/Project/learn/Yolov5_DeepSort_Pytorch");
//    QString command = "D:/GoodGoodStudyDayDayUp/DaSanShang/DigitalImagePropressing/Project/learn/Yolov5_DeepSort_Pytorch/run.bat";
//    p.start(command);
//    p.waitForFinished();
}


void MainWindow::on_pushButton_merge_clicked()
{
    int idx1 = ui->comboBox_id->currentText().toInt();
    int idx2 = mergeDialog->exec();
    int id_new = mergeId(idx1, idx2);
    qDebug() <<"id2" << idx2 << "id new" << id_new;
    if (id_new!=idx1){
        ui->comboBox_id->setCurrentText(QString::number(id_new));
    }
    updateChart();
    update();
}

void MainWindow::getPlayerInfo(int idx, QString &name, QString &team, QString &number, QImage &pic){
    name = players[idx].name;
    team = players[idx].team;
    number = QString::number(players[idx].number);
    pic = players[idx].pic;
}

void MainWindow::getPlayersInfo(QString type, QList<int>& id, QList<qreal>& data){
    /* 获得运动员们的信息
     * type可以是MaxSped, AvgSpeed, Distance
     */
    if (type == "MaxSpeed"){
        for (int i=0; i<players.length(); ++i)
            if (players[i].valid > 0){
                id.append(i);
                data.append(players[i].max_speed);
            }
        return;
    }
    if (type == "AvgSpeed"){
        for (int i=0; i<players.length(); ++i)
            if (players[i].valid > 0){
                id.append(i);
                data.append(players[i].max_distance*1000./(players[i].lastAppeared_time_ms - players[i].firstAppeared_time_ms));
            }
        return;
    }
    if (type == "Distance"){
        for (int i=0; i<players.length(); ++i)
            if (players[i].valid > 0){
                id.append(i);
                data.append(players[i].max_distance);
            }
        return;
    }
}

void MainWindow::on_pushButton_comparation_clicked()
{
    barChartDialog->exec();
}

