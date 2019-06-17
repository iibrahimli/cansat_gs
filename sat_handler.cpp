#include "sat_handler.h"
#include <QDebug>


sat_handler::sat_handler(const QString port_name, qint32 baud)
{
    serial = new QSerialPort();

    // initialize serial port
    serial->setPortName(port_name);
    serial->setBaudRate(baud);
    if (!serial->open(QIODevice::ReadWrite)){
        std::runtime_error(QString("Couldn't open serial port %1").arg(port_name).toStdString());
    }

    // initialize log file name
    tlm_log_filename = QString("%1.csv").arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss"));

    connect(serial, &QSerialPort::readyRead,     this, &sat_handler::read_data);
    connect(serial, &QSerialPort::errorOccurred, this, &sat_handler::handle_error);

}


sat_handler::~sat_handler()
{
    delete serial;
}


/*
 *
 */
void sat_handler::reset_state()
{
    img_counter = 0;
    chunk_counter = 0;

    flight_time.clear();
    packet_id.clear();
    altitude.clear();
    speed.clear();
    img_taken.clear();
    gps.clear();
    img.clear();

    s_read.clear();
    serial->clear();

    tlm_log_filename = QString("%1.csv")
                       .arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss"));
}


void sat_handler::set_port(QString port)
{
    if(port.isEmpty()) return;
    if(serial->isOpen()) serial->setPortName(port);
    else qDebug() << "Serial is not open";
}


void sat_handler::set_baud(qint32 baud)
{
    if(serial->isOpen()) serial->setBaudRate(baud);
    else qDebug() << "Serial is not open";
}


// return # of dropped packets so far, which is
// # of transmitted - # of received
int sat_handler::get_lost_tlm_count()
{
    if(packet_id.size() == 0)
        return 0;
    else
        return packet_id[packet_id.size()-1]+1 - packet_id.size();
}


// return # of received telemetry packets
int sat_handler::get_recv_tlm_count()
{
    return packet_id.size();
}


/*
 *  send data to sat
 */
void sat_handler::send_packet(QByteArray data)
{
    serial->write(data);
}


/*
 *
 */
void sat_handler::read_data()
{
    static QByteArray buf;
    static QByteArray res;                  // result: packet with header
    static int        idx         = 0;      // index in buffer
    static int        header_start_idx = 0;
    static bool       started     = false;  // started reading a packet ('T' encountered)
    static byte       packet_type = 'U';    // TLM_START_BYTE or IMG_START_BYTE
    static byte       size        = 0;      // packet size (with header)

    static int        pckt_id;
    static double     ftime, alt, spd;
    static char       imgt;
    static gps_cd     gpsc;


    // ==========================================
    // =======   TODO: add IMG packets    =======
    // ==========================================

    while (1) {

        if(!started){
            // wait for at least the header
            if(serial->bytesAvailable() < HEADER_SIZE) return;

            // at this point, at least HEADER_SIZE bytes are available,
            // but that might be part of a corrupted packet
            // remove (if necessary) until start byte is found
            buf.append(serial->readAll());

            while(idx < buf.size() && buf[idx] != TLM_START_BYTE) ++idx;
            if(idx == buf.size()){
                // buf does not contain a header
                buf.clear();
                idx = 0;
                return;
            }
            // buf contains a header starting at idx
            size = buf[++idx];
            header_start_idx = idx-1;
            started = true;
        }

        // wait for the body
        if(serial->bytesAvailable() < size - HEADER_SIZE) return;

        // read the body
        buf.append(serial->read(size - HEADER_SIZE));

        // check packet integrity
        if(buf[header_start_idx + size - 1] != TLM_END_BYTE){
            // corrupted packet

            buf.remove(0, header_start_idx + size);
            idx = 0;
            header_start_idx = 0;
            started = false;
            size = 0;

            continue;
        }


        // packet is okay
        res = buf.mid(header_start_idx, size);
        buf.remove(0, header_start_idx + size);
        idx = 0;
        header_start_idx = 0;

        std::cout << res.toStdString() << std::endl;

        if(parse_telemetry(res, ftime, pckt_id, alt, spd, imgt, gpsc)){
            // successfully parsed: telemetry packet complete

            flight_time.emplace_back(ftime);
            packet_id.emplace_back(pckt_id);
            altitude.emplace_back(alt);
            speed.emplace_back(spd);
            img_taken.emplace_back(imgt);
            gps.emplace_back(gpsc);

            // append to the end of the log file
            QFile log(tmp_path + tlm_log_filename);
            if(log.open(QIODevice::ReadWrite | QIODevice::Append)){
                // write starting from the data itself
                log.write(res.mid(3) + QByteArray("\n"));
                log.close();
            }
            else{
                std::cerr << "Couldn't open file " << log.fileName().toStdString() << std::endl;
                return;
            }

            emit push_telemetry(ftime, pckt_id, alt, spd, imgt, gpsc);
        }

        res.clear();
        started  = false;
    }
}


/*
 *
 */
void sat_handler::handle_error(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError){
        std::runtime_error(QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(serial->portName())
                            .arg(serial->errorString()).toStdString());
    }
}


/*
 *
 */
bool sat_handler::parse_telemetry(const QByteArray&  tlm,
                                  double            &ftime,
                                  int               &pckt_id,
                                  double            &alt,
                                  double            &spd,
                                  char              &imgt,
                                  gps_cd            &gpsc)
{
    //  Ts,4318,112.35,112,211.25,105.19,40.15648,N,51.23284,E,0$
    //    |    |      |   |      |      |        | |        | |
    //  -> 0 | 1  |   2  | 3 |   4  |  5   |    6   |7|   8    |9|10
    //
    // 0: header = 'T' (1 byte, ASCII) + total packet size (1 byte, binary)
    // 1: team id (4 bytes, ASCII "4318")
    // 2: elapsed time (ASCII double .2)
    // 3: packet id (ASCII int)
    // 4: altitude (ASCII double .2)
    // 5: speed (ASCII double .2)
    // 6: latitude (ASCII double .5)
    // 7: latitude dir (ASCII char N/S)
    // 8: longitude (ASCII double .5)
    // 9: longitude dir (ASCII char E/W)
    //10: image taken (ASCII char 0/1)


    // split into fields
    QList<QByteArray> spl = tlm.mid(3).split(',');

    // check number of fields
    if(spl.size() != 10)
        return false;

    if(QString(spl[0]).compare("4318") != 0)
        return false;

    ftime   = spl[1].toDouble();
    pckt_id = spl[2].toInt();
    alt     = spl[3].toDouble();
    spd     = spl[4].toDouble();
    gpsc    = gps_cd(spl[5].toDouble(),
                     spl[6][0],
                     spl[7].toDouble(),
                     spl[8][0]);
    imgt    = spl[9][0];

    return true;


//    QTextStream in(tlm);
//    char luluw;
//    int luluw2;

//    // skip T, size, and team ID
//    in >> luluw;
//    in >> luluw;
//    in >> luluw2;

//    in >> ftime;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
//    in >> luluw;

//    in >> pckt_id;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
//    in >> luluw;

//    in >> alt;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
//    in >> luluw;

//    in >> spd;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
//    in >> luluw;

//    in >> gpsc;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
//    in >> luluw;

//    in >> imgt;
////    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

//    return true;
}


/*
 *
 */
bool sat_handler::parse_image(const QByteArray &pckt)
{
    //  Ps,i,c,s,DDDD ... DDDD
    //    | | | |
    //  0 |1|2|3|     4
    //
    // 0: header: 'P' (1 byte, ASCII) and total packet size (1 byte, binary)
    // 1: image id (1 byte, binary)
    // 2: chunk id (1 byte, binary)
    // 3: size of data (4) (1 byte, binary) ?????????????????????????????????????????????????? TODO ===

    // write image to file
    QFile file(img_path + QString("%1.pgm").arg(img_counter));
    if(file.open(QIODevice::WriteOnly)){
        file.write(pckt);
        file.close();
    }
    else{
        std::cerr << "Couldn't open file " << file.fileName().toStdString() << std::endl;
        return false;
    }

    img.emplace_back(file.fileName());
    ++img_counter;

    // send image to MainWindow
    emit push_img(img[img_counter-1]);

    return true;
}
