#ifndef GPS_CD_H
#define GPS_CD_H


#include <string>
#include <iostream>
#include <sstream>


// basic GPS coordinates class
class gps_cd{

public:
    gps_cd();
    explicit gps_cd(std::string str);
    gps_cd(float lat, char latp, float lon, char lonp);
    std::string latitude(std::string sep = " ") const;      // ex: "45.6384 N"
    std::string longitude(std::string sep = " ") const;     // ex: "50.1932 E"
    explicit operator std::string(void) const;              // cast to string

private:
    float _latitude;
    char  _lat_pos;     // N(orth) or S(outh)
    float _longitude;
    char  _lon_pos;     // E(ast) or W(est)

};


#endif // GPS_CD_H
