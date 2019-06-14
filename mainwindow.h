#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPortInfo>
#include "sat_handler.h"


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void init_plots();
    void reset_plots();


    void update_timer();
    void reset_timer();


    /*
     *  add telemetry data to the plots
     */
    void add_telemetry(double pres,
                       double alt,
                       double speed,
                       bool   img_taken,
                       gps_cd coord);


    /*
     *  add photo to the slideshow
     */
    void add_img(QPixmap img);


    /*
     *  slideshow controls
     */
    void on_img_left_clicked();
    void on_img_right_clicked();


    /*
     *  update available serial ports combo box on each click
     */
    void search_serial_ports();


private:

    Ui::MainWindow        *ui;
    sat_handler           *sh;
    std::vector<QPixmap>   imgs;

};


#endif // MAINWINDOW_H
