#include "sat_handler.h"


sat_handler::sat_handler(const QString port_name, qint32 baud)
{
    // initialize image buffer
    img_file_buf = new byte[PGM_HEADER_SIZE + IMG_WIDTH * IMG_HEIGHT];

    // initialize serial port
    serial.setPortName(port_name);
    serial.setBaudRate(baud);

    if (!serial.open(QIODevice::ReadOnly)){
        std::runtime_error(QString("Couldn't open serial port %1").arg(port_name).toStdString());
    }

    connect(&serial, &QSerialPort::readyRead,     this, &sat_handler::read_data);
    connect(&serial, &QSerialPort::errorOccurred, this, &sat_handler::handle_error);

}


sat_handler::~sat_handler()
{
    delete[] img_file_buf;
}


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

    delete[] img_file_buf;
    img_file_buf = nullptr;

    s_read.clear();
    serial.clear();
}


void sat_handler::set_port(const QString &port)
{
    serial.setPortName(port);
}


void sat_handler::set_baud(qint32 baud)
{
    serial.setBaudRate(baud);
}


void sat_handler::read_data()
{
    static QByteArray line;
    static bool       header_read = false;
    static byte       start_byte = 0;
    static byte       size = 0;

    static int        pckt_id;
    static double     ftime, alt, spd;
    static char       imgt;
    static gps_cd     gpsc;


    // TODO: BUFFER GROWING CHECK FOR T or $
    // TODO: check size

//    qDebug() << serial.bytesAvailable();

    if(serial.bytesAvailable() < HEADER_SIZE) return;

//    qDebug() << "Read HEADER_SIZE bytes";

    if(!header_read){

//        qDebug() << "Reading header";

        line.append(serial.read(HEADER_SIZE));
        QDataStream out(line);
        out >> start_byte;
        if(Q_UNLIKELY(start_byte != TLM_START_BYTE)) return;

//        qDebug() << "TLM start byte correct";

        out >> size;
        header_read = true;
    }

    if(Q_LIKELY(size - HEADER_SIZE > serial.bytesAvailable())) return;

//    qDebug() << "Read size bytes except header";

    line.append(serial.read(size));

    std::cout << line.toStdString() << std::endl;

    if(parse_telemetry(line, ftime, pckt_id, alt, spd, imgt, gpsc)){
        // successfully parsed: telemetry packet complete
        push_telemetry(ftime, pckt_id, alt, spd, imgt, gpsc);
        flight_time.emplace_back(ftime);
        packet_id.emplace_back(pckt_id);
        altitude.emplace_back(alt);
        speed.emplace_back(spd);
        img_taken.emplace_back(imgt);
        gps.emplace_back(gpsc);

        emit push_telemetry(ftime, pckt_id, alt, spd, imgt, gpsc);
    }

    line.clear();
    header_read = false;
}


void sat_handler::handle_error(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError){
        std::runtime_error(QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(serial.portName())
                            .arg(serial.errorString()).toStdString());
    }
}


bool sat_handler::parse_telemetry(QByteArray  tlm,
                                  double     &ftime,
                                  int        &pckt_id,
                                  double     &alt,
                                  double     &spd,
                                  char       &imgt,
                                  gps_cd     &gpsc)
{
    // Ts,4318,12.35,11,211.25,105.19,40.15648N,51.23284E,0

    if(tlm[0] != 'T') return false;

    QTextStream in(tlm);
    char luluw;
    int luluw2;

    // skip T, size, and team ID
    in >> luluw;
    in >> luluw;
    in >> luluw2;

    in >> ftime;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> pckt_id;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> alt;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> spd;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> gpsc;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> imgt;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> luluw;
    if(luluw != TLM_END_BYTE) return false;

    return true;
}
