#ifndef SAT_HANDLER_H
#define SAT_HANDLER_H


#include <QObject>
#include <QPixmap>
#include <QtMath>
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QDateTime>
#include <iostream>
#include <vector>
#include "gps_cd.h"


using byte = uint8_t;


#define PGM_HEADER_SIZE  15
#define IMG_WIDTH        480
#define IMG_HEIGHT       480

#define TEAM_ID          4318
#define HEADER_SIZE      3
#define TLM_START_BYTE   'T'
#define TLM_END_BYTE     '$'
#define TLM_MAX_SIZE     56

#define IMG_START_BYTE   'P'


// path to saved images
const QString img_path = QString("/home/stan/Desktop/cansat_2019/data/img/");
const QString tlm_path = QString("/home/stan/Desktop/cansat_2019/data/tlm/");
const QString tmp_path = QString("/home/stan/Desktop/cansat_2019/data/tmp/");


/*
 *  satellite handler
 *  reads data from serial port and sends it to MainWindow
 */
class sat_handler : public QObject{

    Q_OBJECT

public:

    explicit sat_handler(QString port_name,
                         qint32 baud = 230400);

    ~sat_handler();


    /*
     *  resets counters, clears all data and buffers
     */
    void reset_state();


    /*
     *  settings
     */
    void set_port(const QString& port);

    void set_baud(qint32 baud);


    /*
     *  returns number of dropped telemetry packets
     */
    int get_lost_tlm_count();


    /*
     *  returns number of telemetry packets in memory
     */
    int get_recv_tlm_count();


signals:

    /*
     *  adds latest data to plots in MainWindow
     */
    void push_telemetry(double ftime,
                        int    packet_id,
                        double alt,
                        double spd,
                        char   img_taken,
                        gps_cd coord);


    /*
     *  adds latest image to pixmap array in MainWindow
     */
    void push_img(QPixmap img);


private slots:

    /*
     *  reads data from serial port
     */
    void read_data();


    /*
     *  handle serial port errors
     */
    void handle_error(QSerialPort::SerialPortError serialPortError);


private:

    /*
     *  parses telemetry string into references,
     *  appends the line to csv file
     *  returns true if successful
     */
    bool parse_telemetry(const QByteArray&  tlm,
                         double            &ftime,
                         int               &pckt_id,
                         double            &alt,
                         double            &spd,
                         char              &imgt,
                         gps_cd            &gpsc);


    /*
     *  parses an image packet and saves it to the
     *  specified path on disk as <img_counter>.pgm
     *  and increments img_counter
     */
    bool parse_image(const QByteArray& pckt);


    int                   img_counter   {0};  // latest image id
    int                   chunk_counter {0};  // latest chunk id

    // indexes are packet IDs
    std::vector<double>   flight_time;
    std::vector<int>      packet_id;
    std::vector<double>   altitude;
    std::vector<double>   speed;
    std::vector<char>     img_taken;
    std::vector<gps_cd>   gps;

    // holds received (hopefully) pictures
    // in file system, photos are named 0.pgm, 1.pgm, 2.pgm etc
    std::vector<QPixmap>  img;

    QSerialPort           serial;
    QByteArray            s_read;   // raw data read from serial port

    QString               tlm_log_filename; // (datetime.csv) latest file being written

};


#endif // SAT_HANDLER_H
