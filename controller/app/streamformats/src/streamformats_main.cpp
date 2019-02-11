/**
 * streamformats_main.cpp
 *
 * test app to verify streamformat compatibility
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>
#include <cinttypes>

#include <stdexcept>
#include "streamformats.h"


using namespace std;

extern "C" void notification_callback(void * user_obj, int32_t notification_type, uint64_t entity_id, uint16_t cmd_type,
                                      uint16_t desc_type, uint16_t desc_index, uint32_t cmd_status,
                                      void * notification_id)
{
    if (notification_type == avdecc_lib::COMMAND_TIMEOUT || notification_type == avdecc_lib::RESPONSE_RECEIVED)
    {
        const char * cmd_name;
        const char * desc_name;
        const char * cmd_status_name;

        if (cmd_type < avdecc_lib::CMD_LOOKUP)
        {
            cmd_name = avdecc_lib::utility::aem_cmd_value_to_name(cmd_type);
            desc_name = avdecc_lib::utility::aem_desc_value_to_name(desc_type);
            cmd_status_name = avdecc_lib::utility::aem_cmd_status_value_to_name(cmd_status);
        }
        else
        {
            cmd_name = avdecc_lib::utility::acmp_cmd_value_to_name(cmd_type - avdecc_lib::CMD_LOOKUP);
            desc_name = "NULL";
            cmd_status_name = avdecc_lib::utility::acmp_cmd_status_value_to_name(cmd_status);
        }

        printf("\n[NOTIFICATION] (%s, 0x%" PRIx64 ", %s, %s, %d, %s, %p)\n",
               avdecc_lib::utility::notification_value_to_name(notification_type),
               entity_id,
               cmd_name,
               desc_name,
               desc_index,
               cmd_status_name,
               notification_id);
    }
    else
    {
        printf("\n[NOTIFICATION] (%s, 0x%" PRIx64 ", %d, %d, %d, %d, %p)\n",
               avdecc_lib::utility::notification_value_to_name(notification_type),
               entity_id,
               cmd_type,
               desc_type,
               desc_index,
               cmd_status,
               notification_id);
    }

    fflush(stdout);
}

extern "C" void acmp_notification_callback(void * user_obj, int32_t notification_type, uint16_t cmd_type,
                                           uint64_t talker_entity_id, uint16_t talker_unique_id,
                                           uint64_t listener_entity_id, uint16_t listener_unique_id,
                                           uint32_t cmd_status, void * notification_id)
{
}

extern "C" void log_callback(void * user_obj, int32_t log_level, const char * log_msg, int32_t time_stamp_ms)
{
    printf("\n[LOG] %s (%s)\n", avdecc_lib::utility::logging_level_value_to_name(log_level), log_msg);

    fflush(stdout);
}

int main(int argc, char * argv[])
{
    char * interface = NULL;
    int c = 0;

    streamformats_app streamformats_app_ref(notification_callback, log_callback);

    std::vector<std::string> input_argv;
    size_t pos = 0;
    bool done = false;
    std::string cmd_input_orig;

    std::cout << "\nPress Enter to list entites, select an entity with `select x`." << std::endl;
    std::cout << "Valid stream formats can be listed with `formats`." << std::endl;
    std::cout << "Compatibility check is done with `check <sampling_rate`, e.g: `check 48000`." << std::endl;

    while (!done)
    {
        std::string cmd_input;
        printf("\n>");
        std::getline(std::cin, cmd_input);
        cmd_input_orig = cmd_input;

        while ((pos = cmd_input.find(" ")) != std::string::npos)
        {
            if (pos)
                input_argv.push_back(cmd_input.substr(0, pos));
            cmd_input.erase(0, pos + 1);
        }

        if (cmd_input.length() && cmd_input != " ")
        {
            input_argv.push_back(cmd_input);
        }

        if (input_argv.empty())
        {
            streamformats_app_ref.cmd_list();
        }
        else
        {
            std::cout << "input: " << input_argv[0] << "\n";

            if (input_argv[0] == "select")
            {
                if (input_argv.size() < 2) {
                    std::cout << "use: `select x` to select an entity\n";
                } else {
                    streamformats_app_ref.cmd_select(input_argv[1]);
                }
            }
            else if (input_argv[0] == "formats")
            {
                streamformats_app_ref.cmd_formats();
            }
            else if (input_argv[0] == "current")
            {
                streamformats_app_ref.cmd_current();
            }
            else if (input_argv[0] == "check")
            {
                if (input_argv.size() < 2) {
                    std::cout << "use: `check <sampling_rate>` to check stream format compatibility\n";
                } else {
                    streamformats_app_ref.cmd_check(input_argv[1]);
                }
            }
            else if (input_argv[0] == "q")
            {
                std::cout << "See you!\n";
                done = true;
            }
        }


        input_argv.clear();
    }
    return 0;
}
