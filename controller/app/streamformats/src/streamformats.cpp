/**
 * streamformats.cpp
 *
 * streamformats test app command line processing class
 */

#include <assert.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <string.h>

#include "end_station.h"
#include "entity_descriptor.h"
#include "configuration_descriptor.h"
#include "audio_unit_descriptor.h"
#include "stream_input_descriptor.h"
#include "stream_output_descriptor.h"
#include "jack_input_descriptor.h"
#include "jack_output_descriptor.h"
#include "avb_interface_descriptor.h"
#include "clock_source_descriptor.h"
#include "memory_object_descriptor.h"
#include "locale_descriptor.h"
#include "strings_descriptor.h"
#include "stream_port_input_descriptor.h"
#include "stream_port_output_descriptor.h"
#include "audio_cluster_descriptor.h"
#include "audio_map_descriptor.h"
#include "clock_domain_descriptor.h"
#include "external_port_input_descriptor.h"
#include "external_port_output_descriptor.h"
#include "descriptor_field.h"
#include "descriptor_field_flags.h"

#include "util.h"

#include "streamformats.h"

streamformats_app::streamformats_app(void (*notification_callback)(void *, int32_t, uint64_t, uint16_t, uint16_t, uint16_t, uint32_t, void *),
                   void (*log_callback)(void *, int32_t, const char *, int32_t))
{
    netif = avdecc_lib::create_net_interface();
    controller_obj = avdecc_lib::create_controller(netif, notification_callback, NULL, log_callback, 3);
    controller_obj->apply_end_station_capabilities_filters(avdecc_lib::ENTITY_CAPABILITIES_GPTP_SUPPORTED |
                                                               avdecc_lib::ENTITY_CAPABILITIES_AEM_SUPPORTED,
                                                           0, 0);

    sys = avdecc_lib::create_system(avdecc_lib::system::LAYER2_MULTITHREADED_CALLBACK, netif, controller_obj);
    atomic_cout << "AVDECC Controller version: " << controller_obj->get_version() << std::endl;
    print_interfaces_and_select();
    sys->process_start();
}

streamformats_app::~streamformats_app()
{
    sys->process_close();
    sys->destroy();
    controller_obj->destroy();
    netif->destroy();
    // ofstream_ref.close();
}

std::string streamformats_app::qprintable_encode(const char * input_cstr)
{
    std::string input_str(input_cstr);
    std::string output;

    char byte;
    const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    for (size_t i = 0; i < input_str.length() ; ++i)
    {
        byte = input_str[i];

        if ((byte == 0x20) || ((byte >= 33) && (byte <= 126) && (byte != 61)))
        {
            output += byte;
        }
        else
        {
            output += '=';
            output += hex[((byte >> 4) & 0x0F)];
            output += hex[(byte & 0x0F)];
        }
    }

    return output;
}


int streamformats_app::print_interfaces_and_select()
{
    int interface_num = -1;

    for (uint32_t i = 1; i < netif->devs_count() + 1; i++)
    {
        size_t dev_index = i - 1;
        char * dev_desc = netif->get_dev_desc_by_index(dev_index);

        printf("%d (%s)", i, dev_desc);

        uint64_t dev_mac = netif->get_dev_mac_addr_by_index(dev_index);
        if (dev_mac)
        {
            avdecc_lib::utility::MacAddr mac(dev_mac);
            char mac_str[20];
            mac.tostring(mac_str);
            printf(" (%s)", mac_str);
        }

        size_t ip_addr_count = netif->device_ip_address_count(dev_index);
        if (ip_addr_count > 0)
        {
            for(size_t ip_index = 0; ip_index < ip_addr_count; ip_index++)
            {
                const char * dev_ip = netif->get_dev_ip_address_by_index(dev_index, ip_index);
                if (dev_ip)
                    printf(" <%s>", dev_ip);
            }
        }
        printf("\n");
    }

    atomic_cout << "Enter the interface number (1-" << std::dec << netif->devs_count() << "): ";
    std::cin >> interface_num;

    if (interface_num == -1 ||
        interface_num > (int)netif->devs_count() + 1)
    {
        printf("No interfaces found\n");
        exit(EXIT_FAILURE);
    }

    netif->select_interface_by_num(interface_num);

    return 0;
}

int streamformats_app::get_current_end_station(avdecc_lib::end_station ** end_station) const
{
    if (current_end_station >= controller_obj->get_end_station_count())
    {
        atomic_cout << "No End Stations available" << std::endl;
        *end_station = NULL;
        return 1;
    }

    *end_station = controller_obj->get_end_station_by_index(current_end_station);
    return 0;
}

int streamformats_app::get_current_entity_and_descriptor(avdecc_lib::end_station * end_station,
                                                avdecc_lib::entity_descriptor ** entity, avdecc_lib::configuration_descriptor ** configuration)
{
    *entity = NULL;
    *configuration = NULL;

    uint16_t current_entity = end_station->get_current_entity_index();
    if (current_entity >= end_station->entity_desc_count())
    {
        atomic_cout << "Current entity not available" << std::endl;
        return 1;
    }

    *entity = end_station->get_entity_desc_by_index(current_entity);

    uint16_t current_config = end_station->get_current_config_index();
    if (current_config >= (*entity)->config_desc_count())
    {
        atomic_cout << "Current configuration not available" << std::endl;
        return 1;
    }

    *configuration = (*entity)->get_config_desc_by_index(current_config);

    return 0;
}

int streamformats_app::get_current_end_station_entity_and_descriptor(avdecc_lib::end_station ** end_station,
                                                            avdecc_lib::entity_descriptor ** entity, avdecc_lib::configuration_descriptor ** configuration)
{
    if (get_current_end_station(end_station))
        return 1;

    if (get_current_entity_and_descriptor(*end_station, entity, configuration))
    {
        atomic_cout << "Current End Station not fully enumerated" << std::endl;
        return 1;
    }
    return 0;
}

int streamformats_app::cmd_list()
{
    atomic_cout << "\n"
                << "End Station"
                << "  |  "
                << "Name" << std::setw(31) << "  |  "
                << "Entity ID" << std::setw(14) << "  |  "
                << "Firmware Version" << std::setw(19)
                << "  |  "
                << "MAC" << std::endl;
    atomic_cout << std::string(123, '-') << std::endl;

    for (unsigned int i = 0; i < controller_obj->get_end_station_count(); i++)
    {
        avdecc_lib::end_station * end_station = controller_obj->get_end_station_by_index(i);

        if (end_station)
        {
            uint64_t end_station_entity_id = end_station->entity_id();
            avdecc_lib::entity_descriptor_response * ent_desc_resp = NULL;
            if (end_station->entity_desc_count())
            {
                uint16_t current_entity = end_station->get_current_entity_index();
                ent_desc_resp = end_station->get_entity_desc_by_index(current_entity)->get_entity_response();
            }
            const char * end_station_name = "";
            const char * fw_ver = "";
            if (ent_desc_resp)
            {
                end_station_name = (const char *)ent_desc_resp->entity_name();
                fw_ver = (const char *)ent_desc_resp->firmware_version();
            }
            uint64_t end_station_mac = end_station->mac();
            atomic_cout << (std::stringstream() << end_station->get_connection_status()
                                                << std::setw(10) << std::dec << std::setfill(' ') << i << "  |  "
                                                << std::setw(30) << std::hex << std::setfill(' ') << (ent_desc_resp ? qprintable_encode(end_station_name) : "UNKNOWN") << "  |  0x"
                                                << std::setw(16) << std::hex << std::setfill('0') << end_station_entity_id << "  |  "
                                                << std::setw(30) << std::hex << std::setfill(' ') << (ent_desc_resp ? qprintable_encode(fw_ver) : "UNKNOWN") << "  |  "
                                                << std::setw(12) << std::hex << std::setfill('0') << end_station_mac)
                               .rdbuf()
                        << std::endl;
            delete (ent_desc_resp);
        }
    }

    atomic_cout << "\nC - End Station Connected." << std::endl;
    atomic_cout << "D - End Station Disconnected." << std::endl;

    return 0;
}

int streamformats_app::cmd_select(std::string arg)
{
    uint32_t new_end_station = strtoul(arg.c_str(), NULL, 10);
    do_select(new_end_station, 0, 0);
    return 0;
}

int streamformats_app::do_select(uint32_t new_end_station, uint16_t new_entity, uint16_t new_config)
{
    if (is_setting_valid(new_end_station, new_entity, new_config)) // Check if the new setting is valid
    {
        avdecc_lib::end_station * end_station = controller_obj->get_end_station_by_index(new_end_station);
        uint16_t current_entity = end_station->get_current_entity_index();
        avdecc_lib::entity_descriptor_response * entity_desc_resp = end_station->get_entity_desc_by_index(current_entity)->get_entity_response();

        current_end_station = new_end_station;
        end_station->set_current_entity_index(new_entity);
        end_station->set_current_config_index(new_config);
        atomic_cout << "New selection" << std::endl;
        atomic_cout << "\tEnd Station Index:   " << std::dec << new_end_station << std::endl;
        atomic_cout << "\tEnd Station:         " << entity_desc_resp->entity_name()
                    << " (0x" << std::setw(16) << std::hex << std::setfill('0') << end_station->entity_id()
                    << ")" << std::endl;
        atomic_cout << "\tEntity Index:        " << std::dec << new_entity << std::endl;
        atomic_cout << "\tConfiguration Index: " << std::dec << new_config << std::endl;

        delete entity_desc_resp;
    }
    else
    {
        atomic_cout << "Invalid new selection" << std::endl;
    }

    return 0;
}

int streamformats_app::cmd_formats()
{
    avdecc_lib::end_station * end_station;
    avdecc_lib::entity_descriptor * entity;
    avdecc_lib::configuration_descriptor * configuration;

    if (get_current_end_station_entity_and_descriptor(&end_station, &entity, &configuration))
        return 0;
    avdecc_lib::stream_input_descriptor * stream_input_desc_ref = configuration->get_stream_input_desc_by_index(0);
    avdecc_lib::stream_input_descriptor_response * stream_input_resp_ref = stream_input_desc_ref->get_stream_input_response();

    for (int i = 0; i < stream_input_resp_ref->number_of_formats(); i++)
    {
        uint64_t current_format_value = stream_input_resp_ref->get_supported_stream_fmt_by_index(i);
        const char * current_format_name = avdecc_lib::utility::ieee1722_format_value_to_name(current_format_value);
        if (strcmp(current_format_name, "UNKNOWN"))
        {
            atomic_cout << "\tstream_format_" << i << " = " << current_format_name << std::endl;
        }
        else
        {
            atomic_cout << "\tstream_format_" << i << " = 0x" << std::hex << current_format_value << std::endl;
        }
    }

    return 0;
}

int streamformats_app::cmd_current()
{
    avdecc_lib::end_station * end_station;
    avdecc_lib::entity_descriptor * entity;
    avdecc_lib::configuration_descriptor * configuration;

    if (get_current_end_station_entity_and_descriptor(&end_station, &entity, &configuration))
        return 0;
    avdecc_lib::stream_input_descriptor * stream_input_desc_ref = configuration->get_stream_input_desc_by_index(0);
    avdecc_lib::stream_input_descriptor_response * stream_input_resp_ref = stream_input_desc_ref->get_stream_input_response();

    atomic_cout << "current stream format: " << "0x" << std::hex << stream_input_resp_ref->current_format_value()  << std::endl;

    return 0;
}

int streamformats_app::cmd_check(std::string arg)
{
    uint32_t sample_rate = strtoul(arg.c_str(), NULL, 10);
    return do_check(sample_rate);
}

// checks if stream_format defined by sample_rate provided as argument and current channel count
// is compatible with one of the stream_formats list in the entity's AEM
int streamformats_app::do_check(uint32_t sample_rate)
{
    // variable to hold result of check
    uint64_t new_compatible_format = 0;

    // get stream descriptor
    avdecc_lib::end_station * end_station;
    avdecc_lib::entity_descriptor * entity;
    avdecc_lib::configuration_descriptor * configuration;
    if (get_current_end_station_entity_and_descriptor(&end_station, &entity, &configuration))
        return 0;
    avdecc_lib::stream_input_descriptor * stream_input_desc_ref = configuration->get_stream_input_desc_by_index(0);
    avdecc_lib::stream_input_descriptor_response * stream_input_resp_ref = stream_input_desc_ref->get_stream_input_response();

    // read current stream format as raw uint64
    uint64_t current_format_value = stream_input_resp_ref->current_format_value();
    // convert to avdecc-lib stream_format type
    avdecc_lib::utility::ieee1722_stream_format current_format(current_format_value);

    // iterate over allowed stream_formats and
    // check against subtype/channel count/sample rate
    for (int i = 0; i < stream_input_resp_ref->number_of_formats(); i++)
    {
        uint64_t iterator_fmt_value = stream_input_resp_ref->get_supported_stream_fmt_by_index(i);

atomic_cout << "check stream format: " << "0x" << std::hex << current_format_value <<
" " << sample_rate << " " << "0x" << std::hex << iterator_fmt_value << std::endl;

        uint64_t new_format = get_compatible_format(current_format_value, sample_rate, iterator_fmt_value);

        if (new_format > 0) {
            new_compatible_format = new_format;
            break;
        }
    }

    if (new_compatible_format == 0)
    {
        atomic_cout << "No compatible format found" << std::endl;
    }
    else
    {
        atomic_cout << "AEM lists a valid format for " << sample_rate << " Hz." << std::endl <<
        "New stream_format can be set to: " << std::hex << new_compatible_format << std::endl;
    }

    return 0;
}

bool streamformats_app::is_setting_valid(uint32_t end_station, uint16_t entity, uint16_t config)
{
    bool is_setting_valid = (end_station < controller_obj->get_end_station_count()) &&
                            (entity < controller_obj->get_end_station_by_index(end_station)->entity_desc_count()) &&
                            (config < controller_obj->get_end_station_by_index(end_station)->get_entity_desc_by_index(entity)->config_desc_count());

    return is_setting_valid;
}


// 1722 Stream formats
#define IEEE1722_FORMAT_VERSION_SHIFT         (63)
#define IEEE1722_FORMAT_VERSION_MASK          (0x00000001)
#define IEEE1722_FORMAT_SUBTYPE_SHIFT         (56)
#define IEEE1722_FORMAT_SUBTYPE_MASK          (0x0000007f)

// IEC 61883
#define IEC61883_SF_SHIFT                     (55)
#define IEC61883_SF_MASK                      (0x00000001)
#define IEC61883_TYPE_SHIFT                   (49)
#define IEC61883_TYPE_MASK                    (0x0000003f)
#define IEC61883_6_SFC_SHIFT                  (40)
#define IEC61883_6_SFC_MASK                   (0x00000007)
#define IEC61883_6_BLOCK_COUNT_SHIFT          (32)
#define IEC61883_6_BLOCK_COUNT_MASK           (0x000000ff)
#define IEC61883_6_BLOCKING_SHIFT             (31)
#define IEC61883_6_BLOCKING_MASK              (0x00000001)
#define IEC61883_6_NONBLOCKING_SHIFT          (30)
#define IEC61883_6_NONBLOCKING_MASK           (0x00000001)
#define IEC61883_6_UPTO_SHIFT                 (29)
#define IEC61883_6_UPTO_MASK                  (0x00000001)
#define IEC61883_6_SYNCHRONOUS_SHIFT          (28)
#define IEC61883_6_SYNCHRONOUS_MASK           (0x00000001)
#define IEC61883_6_PACKETIZATION_SHIFT        (43)
#define IEC61883_6_PACKETIZATION_MASK         (0x0000001f)
#define IEC61883_6_AM824_IEC60958_COUNT_SHIFT (16)
#define IEC61883_6_AM824_IEC60958_COUNT_MASK  (0x000000ff)
#define IEC61883_6_AM824_MBLA_COUNT_SHIFT     (8)
#define IEC61883_6_AM824_MBLA_COUNT_MASK      (0x000000ff)
#define IEC61883_6_AM824_MIDI_COUNT_SHIFT     (4)
#define IEC61883_6_AM824_MIDI_COUNT_MASK      (0x0000000f)
#define IEC61883_6_AM824_SMPTE_COUNT_SHIFT    (0)
#define IEC61883_6_AM824_SMPTE_COUNT_MASK     (0x0000000f)

#define IEC61883_6_AM824_IEC60958_COMPARE_MASK (((uint64_t)IEC61883_6_BLOCK_COUNT_MASK << IEC61883_6_BLOCK_COUNT_SHIFT) \
    | ((uint64_t)IEC61883_6_AM824_MBLA_COUNT_MASK << IEC61883_6_AM824_MBLA_COUNT_SHIFT) \
    | ((uint64_t)IEC61883_6_SFC_MASK << IEC61883_6_SFC_SHIFT) \
    | ((uint64_t)IEC61883_6_UPTO_MASK << IEC61883_6_UPTO_SHIFT))

// AAF
#define AAF_UPTO_SHIFT                        (52)
#define AAF_UPTO_MASK                         (0x00000001)
#define AAF_NSR_SHIFT                         (48)
#define AAF_NSR_MASK                          (0x0000000f)
#define AAF_TYPE_SHIFT                        (40)
#define AAF_TYPE_MASK                         (0x000000ff)
#define AAF_PCM_BIT_DEPTH_SHIFT               (32)
#define AAF_PCM_BIT_DEPTH_MASK                (0x000000ff)
#define AAF_PCM_CHANNELS_PER_FRAME_SHIFT      (22)
#define AAF_PCM_CHANNELS_PER_FRAME_MASK       (0x000003ff)
#define AAF_PCM_SAMPLES_PER_FRAME_SHIFT       (12)
#define AAF_PCM_SAMPLES_PER_FRAME_MASK        (0x000003ff)

#define AAF_COMPARE_MASK (((uint64_t)AAF_UPTO_MASK << AAF_UPTO_SHIFT) \
    | ((uint64_t)AAF_PCM_CHANNELS_PER_FRAME_MASK << AAF_PCM_CHANNELS_PER_FRAME_SHIFT) \
    | ((uint64_t)AAF_PCM_SAMPLES_PER_FRAME_MASK << AAF_PCM_SAMPLES_PER_FRAME_SHIFT) \
    | ((uint64_t)AAF_NSR_MASK << AAF_NSR_SHIFT))

// checks if two formats and a sample rate are compatible.
// current_format_value: Raw u64 format currently active in the entity
// sample_rate: the sample rate that must be supported by the new format
// format_value: a raw u64 from the stream_formats list of supported formats
uint64_t get_compatible_format(uint64_t current_format_value, uint32_t sample_rate, uint64_t list_format_value)
{
    avdecc_lib::utility::ieee1722_stream_format current_format(current_format_value);
    avdecc_lib::utility::ieee1722_stream_format list_format(list_format_value);

    if (current_format.subtype() == list_format.subtype()) {
        // subtype matches, now check channel count and sampling rate
        switch (current_format.subtype())
        {
        case avdecc_lib::utility::ieee1722_stream_format::IIDC_61883:
        {
            uint64_t current_cmp_val = current_format_value & ~IEC61883_6_AM824_IEC60958_COMPARE_MASK;
            uint64_t list_cmp_val = list_format_value & ~IEC61883_6_AM824_IEC60958_COMPARE_MASK;

            // check if formats are compatible (without sampling rate, channel count and upto flag)
            if (current_cmp_val != list_cmp_val) {
                return 0;
            }

            // as subtypes are the same, we can just instantiate objects for both,
            // the currently set format and the current format in our for loop
            avdecc_lib::utility::iec_61883_iidc_format current_format(current_format_value);
            avdecc_lib::utility::iec_61883_iidc_format list_format(list_format_value);

            // continue if sampling rate does not match
            if (sample_rate != list_format.sample_rate()) {
                return 0;
            }

            // now compare sampling rate
            if (list_format.upto()) {
                // with upto flag set comparison must be >=
                if (list_format.channel_count() >= current_format.channel_count()) {
                    // as avdecc-lib doesn't provide setters, we have to build the format from scratch
                    uint64_t val = uint64_t( (uint64_t) current_format.subtype() << IEEE1722_FORMAT_SUBTYPE_SHIFT) |
                        ((uint64_t) current_format.sf() << IEC61883_SF_SHIFT) |
                        ((uint64_t) current_format.iec61883_type() << IEC61883_TYPE_SHIFT) |
                        ((uint64_t) current_format.packetization_type_value() << IEC61883_6_PACKETIZATION_SHIFT) |
                        // everything taken from current_format except sample_rate
                        ((uint64_t) list_format.fdf_sfc_value() << IEC61883_6_SFC_SHIFT) |
                        ((uint64_t) current_format.dbs() << IEC61883_6_BLOCK_COUNT_SHIFT) |
                        ((uint64_t) current_format.nonblocking() << IEC61883_6_NONBLOCKING_SHIFT) |
                        ((uint64_t) current_format.dbs() << IEC61883_6_AM824_MBLA_COUNT_SHIFT);

                    return val;
                }
            }
            else
            {
                // else it must be an exact match
                if (list_format.channel_count() == current_format.channel_count()) {
                    return list_format.value();
                }
            }
        }
        break;
        case avdecc_lib::utility::ieee1722_stream_format::AAF:
        {
            uint64_t current_cmp_val = current_format_value & ~AAF_COMPARE_MASK;
            uint64_t list_cmp_val = list_format_value & ~AAF_COMPARE_MASK;

            // check if formats are compatible (without sampling rate, channel count and upto flag)
            if (current_cmp_val != list_cmp_val) {
                return 0;
            }

            // same method calls as iec61883, but different type.
            // someone with better c++ skills can surely remove this duplication
            avdecc_lib::utility::aaf_format current_format(current_format_value);
            avdecc_lib::utility::aaf_format list_format(list_format_value);
            // continue if sampling rate does not match
            if (sample_rate != list_format.sample_rate()) {
                return 0;
            }
            // now compare sampling rate
            if (list_format.upto()) {
                // with upto flag set comparison must be >=
                if (list_format.channel_count() >= current_format.channel_count()) {
                    // as avdecc-lib doesn't provide setters, we have to build the format from scratch
                    uint64_t val = uint64_t( (uint64_t) current_format.subtype() << IEEE1722_FORMAT_SUBTYPE_SHIFT) |
                        // everything taken from current_format except sample_rate and samples per frame
                        ((uint64_t) list_format.nsr_value() << AAF_NSR_SHIFT) |
                        ((uint64_t) list_format.samples_per_frame() << AAF_PCM_SAMPLES_PER_FRAME_SHIFT) |
                        ((uint64_t) current_format.packetization_type() << AAF_TYPE_SHIFT) |
                        ((uint64_t) current_format.bit_depth() << AAF_PCM_BIT_DEPTH_SHIFT) |
                        ((uint64_t) current_format.channels_per_frame() << AAF_PCM_CHANNELS_PER_FRAME_SHIFT);

                    return val;
                }
            }
            else
            {
                // else it must be an exact match
                if (list_format.channel_count() == current_format.channel_count()) {
                    return list_format.value();
                }
            }
        }
        break;
        case avdecc_lib::utility::ieee1722_stream_format::CRF:
        {
            avdecc_lib::utility::crf_format current_format(current_format_value);
            avdecc_lib::utility::crf_format list_format(list_format_value);
            if (current_format.base_frequency() == list_format.base_frequency()) {
                return list_format.value();
            }
        }
        }
    }
    return false;
}
