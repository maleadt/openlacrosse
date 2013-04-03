//
// Configuration
//

// Include guard
#ifndef _OPENLACROSSE_WS8610_
#define _OPENLACROSSE_WS8610_

// Standard library
#ifdef USE_PCE
#include "std_includes.hpp"
#else
#include <vector>
#endif

// Boost
#ifdef USE_PCE
#include "boost_includes.hpp"
#else
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/optional.hpp>
#endif
using namespace boost::posix_time;
using namespace boost::local_time;

// Local includes
#include "global.hpp"
#include "station.hpp"
#include "serialinterface.hpp"


//
// Module definitions
//

class WS8610 : public Station
{
public:
    // Construction and destruction
    WS8610(const std::string& portname);

    // Station properties
    unsigned int external_sensors();

    // History management
    HistoryRecord history(int record_no);
    int history_count();
    local_date_time history_modtime();
    HistoryRecord history_first();
    HistoryRecord history_last();
    bool history_reset();

private:
    // Auxiliary
    std::vector<byte> read_safe(address location, size_t length);
    std::vector<byte> memory(address location, size_t length);
    static ptime parse_datetime(const std::vector<byte> &data);
    static boost::optional<double> parse_temperature(const std::vector<byte> &data, int sensor);
    static boost::optional<unsigned int> parse_humidity(const std::vector<byte> &data, int sensor);

    // Communication interface
    SerialInterface _iface;

    // Station characteristics
    unsigned int _external_sensors;
    unsigned int _record_size;
    unsigned int _max_records;
};

#endif
