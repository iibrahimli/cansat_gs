#include "gps_cd.h"


gps_cd::gps_cd()
    :
      _latitude(0.0),
      _lat_pos('U'),
      _longitude(0.0),
      _lon_pos('U')
{}


gps_cd::gps_cd(std::string str){

    // parsing coordinates from a string (ex: "41.1245 N 45.5843 E")
    std::istringstream in(str);

    if(!(in >> _latitude)) std::runtime_error("Could not parse latitude from GPS string");
    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

    if(!(in >> _lat_pos)) std::runtime_error("Could not parse latitude pos from GPS string");
    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

    if(!(in >> _longitude)) std::runtime_error("Could not parse longitude from GPS string");
    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

    if(!(in >> _lon_pos)) std::runtime_error("Could not parse longitude pos from GPS string");
    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

    in.clear();
}


gps_cd::gps_cd(double lat, char latp, double lon, char lonp)
    :
      _latitude(lat),
      _lat_pos(latp),
      _longitude(lon),
      _lon_pos(lonp)
{}


QTextStream& operator >> (QTextStream& in, gps_cd coord){

    char luluw;

    in >> coord._latitude;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> coord._lat_pos;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> coord._longitude;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();
    in >> luluw;

    in >> coord._lon_pos;
//    while(in.peek() == ',' || in.peek() == ' ') in.ignore();

    return in;
}


std::string gps_cd::latitude(std::string sep) const {
    return std::to_string(_latitude) + sep + _lat_pos;
}


std::string gps_cd::longitude(std::string sep) const {
    return std::to_string(_longitude) + sep + _lon_pos;
}


gps_cd::operator std::string(void) const {
    return std::string(latitude() + ", " + longitude());
}
