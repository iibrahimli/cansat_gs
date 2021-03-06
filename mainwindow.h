// In the name of God

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPortInfo>
#include "sat_handler.h"


#define SMOOTHING_WINDOW_SIZE 5


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
    void add_telemetry(double ftime,
                       int    packet_id,
                       double alt,
                       double spd,
                       char   img_taken,
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


    /*
     *  saves telemetry data to a CSV file
     *  prompts user to select path and filename
     */
    void export_csv();


private:

    Ui::MainWindow        *ui;
    sat_handler           *sh;
    QPixmap                logo_pixmap;   // logo to display when there are no images
    std::vector<QPixmap>   imgs;
    int                    img_index = -1;
    bool                   autoscroll_table = true;
    double                 sat_battery = 4500;

    // smoothing
    std::vector<double>    alt_buffer;
    std::vector<double>    spd_buffer;

};


#endif // MAINWINDOW_H
