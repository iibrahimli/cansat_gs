#ifndef SAT_HANDLER_H
#define SAT_HANDLER_H


#include <QObject>
#include <QPixmap>
#include <QtMath>
#include <QtSerialPort/QSerialPort>
#include <iostream>
#include <vector>
#include "gps_cd.h"


using byte = uint8_t;

#define PGM_HEADER_SIZE  15
#define IMG_WIDTH        480
#define IMG_HEIGHT       480
#define TLM_END_CHAR     '$'


/*
 *  satellite handler
 *  reads data from serial port and sends it to MainWindow
 */
class sat_handler : public QObject{

    Q_OBJECT

public:

    explicit sat_handler(QString port_name,
                         QSerialPort::BaudRate baud = QSerialPort::Baud115200);

    ~sat_handler();


    /*
     *  resets counters, clears all data and buffers
     */
    void reset_state();


signals:

    /*
     *  adds latest data to plots in MainWindow
     */
    void push_telemetry(double pres,
                        double alt,
                        double spd,
                        bool   img_taken,
                        gps_cd coord);


    /*
     *  adds latest image to pixmap array in MainWindow
     */
    void push_img(QPixmap img);


private slots:

    /*
     *  reads data from serial into s_read, copies until TLM_END_CHAR char is found
     */
    void read_data();


    /*
     *  handle serial port errors
     */
    void handle_error(QSerialPort::SerialPortError serialPortError);


private:

    /*
     *  parses telemetry string into references,
     *  returns true if successful
     */
    bool parse_telemetry(QByteArray  tlm,
                         double     &pres,
                         double     &alt,
                         double     &spd,
                         bool       &imgt,
                         gps_cd     &gpsc);


    int                   img_counter   {0};  // latest image id
    int                   chunk_counter {0};  // latest chunk id

    // indexes are packet IDs
    std::vector<double>   pressure;
    std::vector<double>   altitude;
    std::vector<double>   speed;
    std::vector<bool>     img_taken;
    std::vector<gps_cd>   gps;

    // holds received (hopefully) pictures
    // in file system, photos are named 0.pgm, 1.pgm, 2.pgm etc
    std::vector<QPixmap>  img;

    // image (PGM) file buffer, will be written to a file
    byte                 *img_file_buf{nullptr};

    QSerialPort           serial;
    QByteArray            s_read;   // raw data read from serial port

};


#endif // SAT_HANDLER_H
