#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    :
      QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(123);

    // plots
    init_plots();

    // table
    ui->table->setColumnCount(8);
    ui->table->setHorizontalHeaderLabels(QStringList() << "Team ID"
                                                       << "Elapsed time"
                                                       << "Packet ID"
                                                       << "Altitude"
                                                       << "Speed"
                                                       << "Latitude"
                                                       << "Longitude"
                                                       << "Photo taken");
    ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    // UI
    ui->refresh->setIcon(QIcon(":/rsc/refresh.png"));
    ui->default_baud->setIcon(QIcon(":/rsc/default_baud.png"));
    ui->baudrate->addItems(QStringList() << "9600"
                                         << "115200"
                                         << "230400"
                                         << "1000000");
    ui->baudrate->setCurrentText("230400");


    // image
    QImage logo_img(":/rsc/logo.png");
    logo_img = logo_img.scaled(ui->aerial->width()/2.5, ui->aerial->height()/2.5);
    for(auto x=0; x<logo_img.width(); ++x){
        for(auto y=0; y<logo_img.height(); ++y){
            QColor col = logo_img.pixelColor(x, y);
            col.setHslF(col.hslHueF(),
                        col.hslSaturationF()/4,
                        col.lightnessF()*1.2,
                        col.alphaF()/3);
            logo_img.setPixelColor(x, y, col);
        }
    }
    QPixmap aepxm;
    aepxm.convertFromImage(logo_img);
    ui->aerial->setPixmap(aepxm);
    ui->aerial->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    ui->photo_num->clear();
    ui->photo_num->setText("0 / 0");

    // ================================
    // == * * * * * * * * * * * * *  ==
    // == TODO: fucking GPS bitmap ? ==
    // == * * * * * * * * * * * * *  ==
    // ================================

    // 1 Hz timer
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &MainWindow::update_timer);
    timer->start(1000);


    // UI signals/slots
    connect(ui->reset,   &QPushButton::clicked, this, &MainWindow::reset_timer);
    connect(ui->refresh, &QPushButton::clicked, this, &MainWindow::search_serial_ports);
    connect(ui->default_baud, &QPushButton::clicked, [=](){
        ui->baudrate->setCurrentText("230400");
        sh->set_baud(ui->baudrate->currentText().toInt());
    });
    connect(ui->ports,
            QOverload<const QString &>::of(&QComboBox::currentIndexChanged),
            [=](){
                sh->set_port(ui->ports->currentText());
            });
    connect(ui->baudrate,
            QOverload<const QString &>::of(&QComboBox::currentIndexChanged),
            [=](){
                sh->set_baud(ui->baudrate->currentText().toInt());
            });


    // satellite handler
    search_serial_ports();
    sh = new sat_handler(ui->ports->currentText());


    // sat_handler signals/slots
    QObject::connect(sh, &sat_handler::push_telemetry, this, &MainWindow::add_telemetry);
    QObject::connect(sh, &sat_handler::push_img,       this, &MainWindow::add_img);


    // ============================================

    emit sh->push_telemetry(12.45, 11, 111.08, 12.60, '0', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(14.45, 13, 114.98, 7.45, '0', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(15.60, 14, 116.65, 5.48, '1', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(16.31, 15, 115.48, 6.99, '0', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(18.40, 16, 117.65, 5.48, '0', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(19.59, 17, 119.00, 5.99, '0', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(23.02, 20, 127.55, 6.48, '1', gps_cd("54.52267,N,47.12375,E"));
    emit sh->push_telemetry(24.44, 21, 128.12, 5.99, '0', gps_cd("54.52267,N,47.12375,E"));

    emit sh->push_img(QPixmap(img_path + "0.png"));
    emit sh->push_img(QPixmap(img_path + "1.jpg"));
    emit sh->push_img(QPixmap(img_path + "1.png"));
    emit sh->push_img(QPixmap(img_path + "2.png"));
    emit sh->push_img(QPixmap(img_path + "3.png"));
    emit sh->push_img(QPixmap(img_path + "4.png"));
    emit sh->push_img(QPixmap(img_path + "6.png"));

}


MainWindow::~MainWindow(){
    delete ui;
    delete sh;
}


void MainWindow::init_plots()
{
    ui->altitude->addGraph();
    ui->speed->addGraph();
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


void MainWindow::add_telemetry(double ftime,
                               int    packet_id,
                               double alt,
                               double spd,
                               char   img_taken,
                               gps_cd coord)
{
    ui->altitude->graph(0)->addData(ftime, alt);
    ui->altitude->rescaleAxes();
    ui->altitude->replot();

    ui->speed->graph(0)->addData(ftime, spd);
    ui->speed->rescaleAxes();
    ui->speed->replot();

    // update table
    ui->table->insertRow(ui->table->rowCount());
    QStringList row = QStringList() << QString("4318")
                                    << QString::number(ftime, 'f', 2)
                                    << QString::number(packet_id)
                                    << QString::number(alt, 'f', 2)
                                    << QString::number(spd, 'f', 2)
                                    << QString::fromStdString(coord.latitude())
                                    << QString::fromStdString(coord.longitude())
                                    << QString(QChar(img_taken));
    for(auto c = 0; c < ui->table->columnCount(); ++c){
        ui->table->setItem(ui->table->rowCount()-1, c, new QTableWidgetItem(row.at(c)));
    }
    ui->table->scrollToBottom();
}


void MainWindow::add_img(QPixmap img)
{
    imgs.push_back(img);
    ++img_index;
    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));
}


// previous image in the array
void MainWindow::on_img_left_clicked()
{
    img_index = (imgs.size() + (img_index - 1)) % imgs.size();
    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));
}


// next image in the array
void MainWindow::on_img_right_clicked()
{
    img_index = (img_index+1) % imgs.size();
    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));
}


void MainWindow::search_serial_ports()
{
    ui->ports->clear();
    for(auto p : QSerialPortInfo::availablePorts())
        ui->ports->addItem(p.portName());
}


void MainWindow::set_sat_handler_settings()
{

}
