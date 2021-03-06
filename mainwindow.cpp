#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

//ADD AUTOSCROLL AND REMOVE $ FROM CSV AND ADD HEADER TO CSV


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
                                         << "460800");
    ui->baudrate->setCurrentText("230400");
    ui->autoscroll->setChecked(true);

    // menu & exporting
    QAction *export_to_csv = new QAction(QIcon(":/rsc/export.png"), "&Export to CSV");
    ui->menuData->addAction(export_to_csv);
    connect(export_to_csv, &QAction::triggered, this, &MainWindow::export_csv);


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
    logo_pixmap.convertFromImage(logo_img);
    ui->aerial->setPixmap(logo_pixmap);
    ui->aerial->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    ui->photo_num->clear();
    ui->photo_num->setText("0 / 0");


    // ===============================
    // ==|*************************|==
    // ==|* TODO: fucking GPS map *|==
    // ==|*************************|==
    // ===============================


    // 1 Hz timer
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &MainWindow::update_timer);
    connect(ui->init, &QPushButton::clicked, [timer](){
        timer->start();
    });
    connect(ui->init, &QPushButton::clicked, [=](){
        sh->send_packet(QByteArray("DATTE"));
    });


    // satellite handler
    search_serial_ports();
    sh = new sat_handler(ui->ports->currentText());


    // sat_handler signals/slots
    connect(sh, &sat_handler::push_telemetry, this, &MainWindow::add_telemetry);
    connect(sh, &sat_handler::push_img,       this, &MainWindow::add_img);



    // UI signals/slots
    connect(ui->reset, &QPushButton::clicked, [=](){
        reset_plots();
        reset_timer();
        timer->stop();
        ui->tlm_pckts->setText(QString("%1 (%2 Hz)").arg("0", "0.00"));
        ui->lost_tlm_pckts->setText(QString("%1 (%2 % )").arg("0", "0.00"));
        ui->img_pckts_freq->setText(QString("%1 Hz").arg("0.00"));
        sh->reset_state();

        // set logo as image
        ui->aerial->setPixmap(logo_pixmap);
        ui->photo_num->setText("0 / 0");
        img_index = -1;
        imgs.clear();

        // reset tables
        ui->table->setRowCount(0);

    });
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
    connect(ui->autoscroll, &QCheckBox::toggled, [=](bool checked){
       autoscroll_table = checked;
    });

    /* SMOOTHING */
    connect(ui->alt_smoothing, &QCheckBox::toggled, [=](bool checked){
        if(checked){
            // smoothing (graph 1 is smoothed version)
            ui->altitude->graph(0)->setPen(QPen(QBrush(Qt::lightGray), 1));
            ui->altitude->graph(1)->setPen(QPen(QBrush(Qt::blue), 2));
            ui->altitude->graph(1)->setVisible(true);
        }
        else{
            // no smoothing
            ui->altitude->graph(0)->setPen(QPen(QBrush(Qt::blue), 1));
            ui->altitude->graph(1)->setVisible(false);
        }
        ui->altitude->replot();
    });
    connect(ui->spd_smoothing, &QCheckBox::toggled, [=](bool checked){
        if(checked){
            // smoothing (graph 1 is smoothed version)
            ui->speed->graph(0)->setPen(QPen(QBrush(Qt::lightGray), 1));
            ui->speed->graph(1)->setPen(QPen(QBrush(Qt::blue), 2));
            ui->speed->graph(1)->setVisible(true);
        }
        else{
            // no smoothing
            ui->speed->graph(0)->setPen(QPen(QBrush(Qt::blue), 1));
            ui->speed->graph(1)->setVisible(false);
        }
        ui->speed->replot();
    });
    /* image percentage */
    connect(sh, &sat_handler::update_img_recv_perc, ui->img_recv, &QProgressBar::setValue);



    // ============================================

//    emit sh->push_telemetry(12.45, 11, 111.08, 2.60, '0', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(14.45, 13, 114.98, 7.45, '0', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(15.60, 14, 116.65, 5.48, '1', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(16.31, 15, 115.48, 6.99, '0', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(18.40, 16, 117.65, 5.48, '0', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(19.59, 17, 119.00, 5.99, '0', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(23.02, 20, 127.55, 6.48, '1', gps_cd("54.52267,N,47.12375,E"));
//    emit sh->push_telemetry(24.44, 21, 128.12, 5.99, '0', gps_cd("54.52267,N,47.12375,E"));

//    emit sh->push_img(QPixmap(img_path + "0.png"));
//    emit sh->push_img(QPixmap(img_path + "1.jpg"));
//    emit sh->push_img(QPixmap(img_path + "1.png"));
//    emit sh->push_img(QPixmap(img_path + "2.png"));
//    emit sh->push_img(QPixmap(img_path + "3.png"));
//    emit sh->push_img(QPixmap(img_path + "4.png"));
//    emit sh->push_img(QPixmap(img_path + "6.png"));

}


MainWindow::~MainWindow(){
    delete ui;
    delete sh;
}


/*
 *  initialize plots
 */
void MainWindow::init_plots()
{
    // 2 graphs because of smoothing
    ui->altitude->addGraph();
    ui->altitude->addGraph();
    ui->altitude->graph(1)->setVisible(false);

    ui->speed->addGraph();
    ui->speed->addGraph();
    ui->speed->graph(1)->setVisible(false);
}


/*
 *  reset plots
 */
void MainWindow::reset_plots()
{
    ui->altitude->graph(0)->data()->clear();
    ui->altitude->graph(1)->data()->clear();
    ui->altitude->replot();

    ui->speed->graph(0)->data()->clear();
    ui->speed->graph(1)->data()->clear();
    ui->speed->replot();
}


/*
 *
 */
void MainWindow::update_timer()
{
    ui->lcdNumber->display(ui->lcdNumber->value()+1);

    // update battery
    sat_battery -= 0.1897222;
    ui->bat_mwh->setText(QString("%1/4500 mWh (%2%)").arg(
                             QString::number(sat_battery,                'f', 0),
                             QString::number(sat_battery / 4500 * 100.0, 'f', 1)));

    // update stats
    ui->tlm_pckts->setText(QString("%1 (%2 Hz)").arg(sh->get_recv_tlm_count() + sh->get_lost_tlm_count())
                               .arg(QString::number((sh->get_recv_tlm_count() + sh->get_lost_tlm_count())
                                                    / ui->lcdNumber->value(), 'f', 2)));
    ui->lost_tlm_pckts->setText(QString("%1 (%2 % )").arg(
                               sh->get_lost_tlm_count()).arg(
                               (sh->get_recv_tlm_count() + sh->get_recv_tlm_count() == 0) ?
                               QString("0") :
                               QString::number((double) sh->get_lost_tlm_count() /
                               (sh->get_lost_tlm_count() + sh->get_recv_tlm_count()) * 100., 'f', 1) ));

    /*
     *  uncomment for fake telemetry after "initialize"
     */
//    emit sh->push_telemetry(ui->lcdNumber->value(),
//                            ui->lcdNumber->value(),
//                            ((double) qrand()/RAND_MAX) * ui->lcdNumber->value()*10 + ui->lcdNumber->value()*3.4,
//                            ((double) qrand()/RAND_MAX) + 3.4,
//                            '0',
//                            gps_cd("54.52267,N,47.12375,E"));
}


/*
 *  tick
 */
void MainWindow::reset_timer()
{
    ui->lcdNumber->display(0);

    sat_battery = 4500;
    ui->bat_mwh->setText(QString("%1/4500 mWh (%2%)").arg(
                             QString::number(4500, 'f', 0),
                             QString::number(100,  'f', 1)));

}


/*
 *  slot to add telemetry to plots and table
 */
void MainWindow::add_telemetry(double ftime,
                               int    packet_id,
                               double alt,
                               double spd,
                               char   img_taken,
                               gps_cd coord)
{
    // add smoothed versions
    if(Q_LIKELY(alt_buffer.size() == SMOOTHING_WINDOW_SIZE)) alt_buffer.erase(alt_buffer.cbegin());
    alt_buffer.push_back(alt);
    ui->altitude->graph(1)->addData(ftime,
                                    std::accumulate(alt_buffer.begin(), alt_buffer.end(), 0.0) / alt_buffer.size());

    if(Q_LIKELY(spd_buffer.size() == SMOOTHING_WINDOW_SIZE)) spd_buffer.erase(spd_buffer.cbegin());
    spd_buffer.push_back(spd);
    ui->speed->graph(1)->addData(ftime,
                                 std::accumulate(spd_buffer.begin(), spd_buffer.end(), 0.0) / spd_buffer.size());


    // update graphs
    ui->altitude->graph(0)->addData(ftime, alt);
    ui->altitude->rescaleAxes();
    ui->altitude->replot();

    ui->speed->graph(0)->addData(ftime, spd);
    ui->speed->rescaleAxes();
    ui->speed->replot();


    // update table
    ui->table->insertRow(ui->table->rowCount());
    QStringList row = QStringList() << QString::number(TEAM_ID)
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
    if(autoscroll_table) ui->table->scrollToBottom();

    // update stats
    ui->tlm_pckts->setText(QString("%1 (%2 Hz)").arg(sh->get_recv_tlm_count() + sh->get_lost_tlm_count())
                               .arg(QString::number((sh->get_recv_tlm_count() + sh->get_lost_tlm_count())
                                                    / ui->lcdNumber->value(), 'f', 2)));
    ui->lost_tlm_pckts->setText(QString("%1 (%2 % )").arg(
                               sh->get_lost_tlm_count()).arg(
                               (sh->get_recv_tlm_count() + sh->get_recv_tlm_count() == 0) ?
                               QString("0") :
                               QString::number((double) sh->get_lost_tlm_count() /
                               (sh->get_lost_tlm_count() + sh->get_recv_tlm_count()) * 100., 'f', 1) ));

}


/*
 *  slot to add image to slideshow
 */
void MainWindow::add_img(QPixmap img)
{
    imgs.push_back(img);
    ++img_index;
//    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));

    // update image frequency
    ui->img_pckts_freq->setText(QString("%1 Hz").arg(
                                    QString::number((double) imgs.size() / ui->lcdNumber->value(), 'f', 2)));
}


// previous image in the array
void MainWindow::on_img_left_clicked()
{
    if(imgs.size() == 0) return;
    img_index = (imgs.size() + (img_index - 1)) % imgs.size();
    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));
}


// next image in the array
void MainWindow::on_img_right_clicked()
{
    if(imgs.size() == 0) return;
    img_index = (img_index+1) % imgs.size();
    ui->aerial->setPixmap(imgs[img_index].scaled(ui->aerial->width(), ui->aerial->height()));
    ui->photo_num->setText(QString("%1 / %2").arg(img_index+1).arg(imgs.size()));
}


/*
 *  search available ports and add to ports combo box
 */
void MainWindow::search_serial_ports()
{
    ui->ports->clear();
    for(auto p : QSerialPortInfo::availablePorts()){
        ui->ports->addItem(p.portName());
    }
}


/*
 *  save telemetry to disk (prompts user)
 */
void MainWindow::export_csv()
{
    // open a modal dialog
    QString file_name = QFileDialog::getSaveFileName(this,
                                                     "Export telemetry",
                                                     tlm_path + QString::number(TEAM_ID) + "_TLM_2019.csv",
                                                     "CSV files (*.csv);;");
    if (file_name.isEmpty())
        return;
    else {
        QFile file(file_name);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, "Unable to open file", file.errorString());
            return;
        }

        // write headers:
        file.write("Team ID,"
              "Elapsed time,"
              "Packet ID,"
              "Altitude,"
              "Speed,"
              "Latitude,"
              "Longitude,"
              "Photo taken\r\n");

        // write data from table
        QTextStream out(&file);
        QStringList strl;

        for(auto r = 0; r < ui->table->rowCount(); ++r){
            for(auto c = 0; c < ui->table->columnCount(); ++c){
                strl << ui->table->item(r, c)->text();
            }
            out << strl.join(",") << "\r\n";
            strl.clear();
        }

        file.close();
    }
}
