#include "sat_handler.h"


sat_handler::sat_handler(QString port_name, QSerialPort::BaudRate baud)
{
    // initialize image buffer
    img_file_buf = new byte[PGM_HEADER_SIZE + IMG_WIDTH * IMG_HEIGHT];

    // initialize serial port
    serial = QSerialPort(port_name);
    serial.setBaudRate(baud);

    if (!serial.open(QIODevice::ReadOnly)){
        std::runtime_error(QString("Couldn't open serial port %1").arg(port_name));
    }

    connect(serial, &QSerialPort::readyRead,     this, &sat_handler::read_data);
    connect(serial, &QSerialPort::errorOccurred, this, &sat_handler::handle_error);

}


sat_handler::~sat_handler()
{
    delete[] img_file_buf;
}


void sat_handler::reset_state()
{
    img_counter = 0;
    chunk_counter = 0;

    pressure.clear();
    altitude.clear();
    speed.clear();
    img_taken.clear();
    gps.clear();
    img.clear();

    delete[] img_file_buf;
    img_file_buf = nullptr;

    s_read.clear();
}


void sat_handler::read_data()
{
    s_read.append(serial->readAll());

    // go through read data until TLM_END_CHAR is found ?
    // add fixed size header to telemetry (first byte 'T', second byte is size)
}


void sat_handler::handle_error(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError){
        m_standardOutput << QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(serial->portName())
                            .arg(serial->errorString())
                         << endl;
        QCoreApplication::exit(1);
    }
}
