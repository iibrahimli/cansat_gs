#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    :
      QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(123);

    // satellite handler
    sh = new sat_handler();

    // sat_handler signals/slots
    QObject::connect(sh, &sat_handler::push_telemetry, this, &MainWindow::add_telemetry);
    QObject::connect(sh, &sat_handler::push_img,       this, &MainWindow::add_img);

    // ---- plots ----
    init_plots();


    // ---- image ----
    QPixmap aepxm("/home/stan/cansat_2019/data/logo.png");
    ui->aerial->setPixmap(aepxm);


    // ---- 1 Hz timer ----
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &MainWindow::update_timer);
    timer->start(1000);

    // button signals/slots
    QObject::connect(ui->reset, &QPushButton::clicked, this, &MainWindow::reset_timer);

}


MainWindow::~MainWindow(){
    delete ui;
}


void MainWindow::init_plots()
{
    ui->altitude->addGraph();
    ui->speed->addGraph();
//    ui->pressure->addGraph();
//    ui->temperature->addGraph();
}


void MainWindow::reset_plots()
{
    ui->altitude->graph(0)->data()->clear();
    ui->altitude->replot();

    ui->speed->graph(0)->data()->clear();
    ui->speed->replot();
}


void MainWindow::update_timer()
{
    ui->lcdNumber->display(ui->lcdNumber->value()+1);
}


void MainWindow::reset_timer()
{
    ui->lcdNumber->display(0);
}



void MainWindow::add_telemetry(double pres,
                               double alt,
                               double speed,
                               bool   img_taken,
                               gps_cd coord)
{
    // starting time in milliseconds (reference)
    static qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    qint64 t_cur = QDateTime::currentMSecsSinceEpoch() - t_start;

    ui->altitude->graph(0)->addData(t_cur, alt);
    ui->altitude->rescaleAxes();
    ui->altitude->replot();

    ui->speed->graph(0)->addData(t_cur, speed);
    ui->speed->rescaleAxes();
    ui->speed->replot();

    ui->pressure->graph(0)->addData(t_cur, pres);
    ui->pressure->rescaleAxes();
    ui->pressure->replot();

//    ui->temperature->graph(0)->addData(t_cur, ???);
//    ui->temperature->rescaleAxes();
//    ui->temperature->replot();

}


void MainWindow::add_img(QPixmap img)
{

}


// previous image in the array
void MainWindow::on_img_left_clicked()
{

}


// next image in the array
void MainWindow::on_img_right_clicked()
{

}
