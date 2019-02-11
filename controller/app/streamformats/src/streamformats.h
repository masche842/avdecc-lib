/**
 * streamformats.h
 *
 * streamformats test app command line processing class
 */

#pragma once
#include <fstream>
#include <sstream>
#include <map>
#include <queue>

#include "net_interface.h"
#include "system.h"
#include "controller.h"
#include "util.h"
#include "enumeration.h"

#include "entity_descriptor.h"


class AtomicOut : public std::ostream
{
public:
    AtomicOut() : std::ostream(0), buffer()
    {
        this->init(buffer.rdbuf());
    }

    ~AtomicOut()
    {
        // Use printf as cout seems to still be interleaved
        printf("%s", buffer.str().c_str());
        fflush(stdout);
    }

private:
    std::ostringstream buffer;
};

#define atomic_cout AtomicOut()

class streamformats_app
{
private:
    avdecc_lib::net_interface * netif;
    avdecc_lib::system * sys;
    avdecc_lib::controller * controller_obj;

    uint32_t current_end_station;

public:
    streamformats_app();

    ///
    /// Constructor for streamformats used for constructing an object with notification and log callback functions.
    ///
    streamformats_app(void (*notification_callback)(void *, int32_t, uint64_t, uint16_t, uint16_t, uint16_t, uint32_t, void *),
             void (*log_callback)(void *, int32_t, const char *, int32_t));

    ~streamformats_app();

private:
    int print_interfaces_and_select();
    int get_current_end_station(avdecc_lib::end_station ** end_station) const;
    int get_current_entity_and_descriptor(avdecc_lib::end_station * end_station,
                                          avdecc_lib::entity_descriptor ** entity, avdecc_lib::configuration_descriptor ** descriptor);
    int get_current_end_station_entity_and_descriptor(avdecc_lib::end_station ** end_station,
                                                      avdecc_lib::entity_descriptor ** entity, avdecc_lib::configuration_descriptor ** configuration);

    ///
    /// Encode a string with Quoted-Printable encoding.
    ///
    std::string qprintable_encode(const char * input_cstr);
public:

    ///
    /// List all valid stream formats.
    ///
    int cmd_formats();

    ///
    /// List current stream format.
    ///
    int cmd_current();

    ///
    /// Test compatibility of current stream format with sampling rate
    ///
    int cmd_check(std::string arg);
    int do_check(uint32_t sample_rate);

    ///
    /// Display a table with information about each end station discovered with ADP.
    ///
    int cmd_list();

    ///
    /// Change selected end station.
    ///
    int cmd_select(std::string arg);
    int do_select(uint32_t new_end_station, uint16_t new_entity, uint16_t new_config);

    ///
    /// Check if end station, entity, and configuration setting is in range and valid.
    ///
    bool is_setting_valid(uint32_t end_station, uint16_t entity, uint16_t config);
};

uint64_t get_compatible_format(uint64_t current_format_value, uint32_t sample_rate, uint64_t format_value);
