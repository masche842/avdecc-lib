/**
 * Testing get_compatible_streamformat function
 */

#include <iostream>
#include "util.h"
#include "streamformats.h"

struct ieee1722_format_test
{
    uint64_t current_value;
    uint64_t list_value;
    uint32_t sample_rate;
    uint64_t expected_value;
};

struct ieee1722_format_test ieee1722_test_table[] =
{
    // CRF AUDIO
    // exact match only
    {UINT64_C(0x041006010000BB80), UINT64_C(0x041006010000BB80), 48000, UINT64_C(0x041006010000BB80)},

    // AAF PCM
    // 48 kHz -> 96 kHz, 8 channels (exact match)
    {UINT64_C(0x0205021802006000), UINT64_C(0x020702180200C000), 96000, UINT64_C(0x020702180200C000)},
    // 48 kHz -> 96 kHz, 2 channels with UT
    {UINT64_C(0x0205021800806000), UINT64_C(0x021702180200c000), 96000, UINT64_C(0x020702180080C000)},

    // IEC61883-6 AM824
    // 44.1 kHz -> 192 kHz, 8 channels (exact match)
    {UINT64_C(0x00a0010840000800), UINT64_C(0x00A0060840000800), 192000, UINT64_C(0x00A0060840000800)},
    // 44.1 kHz -> 192 kHz, 2 channels with UT
    {UINT64_C(0x00a0010240000200), UINT64_C(0x00A0060860000800), 192000, UINT64_C(0x00A0060240000200)},
};

int main()
{
    unsigned int am824_test_table_size = (sizeof(ieee1722_test_table) / sizeof(ieee1722_test_table[0]));
    for (size_t i = 0; i < am824_test_table_size; i++)
    {
        struct ieee1722_format_test * p = &ieee1722_test_table[i];

        // get_compatible_format(uint64_t current_format_value, uint32_t sample_rate, uint64_t format_value);
        uint64_t generated_value = get_compatible_format(p->current_value, p->sample_rate, p->list_value);

        if (generated_value != p->expected_value)
        {
            std::cout << "ERROR: format not valid, "
                " Expected: 0x" << std::hex << p->expected_value << ", Got: 0x" << std::hex << generated_value << std::endl;
            return 1;
        }
    }

    std::cout << "Passed" << std::endl;
    return 0;
}
