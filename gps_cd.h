#ifndef GPS_CD_H
#define GPS_CD_H


#include <QTextStream>
#include <string>
#include <iostream>
#include <sstream>


// basic GPS coordinates class
class gps_cd{

public:
    gps_cd();
    explicit gps_cd(std::string str);
    friend QTextStream& operator >> (QTextStream& in, gps_cd coord); // read from stream
    gps_cd(double lat, char latp, double lon, char lonp);
    std::string latitude(std::string sep = " ") const;               // ex: "45.6384 N"
    std::string longitude(std::string sep = " ") const;              // ex: "50.1932 E"
    explicit operator std::string(void) const;                       // cast to string

private:
    double _latitude;
    char   _lat_pos;     // N(orth) or S(outh)
    double _longitude;
    char   _lon_pos;     // E(ast) or W(est)

};


#endif // GPS_CD_H
