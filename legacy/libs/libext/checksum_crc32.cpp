#include <ext/checksum.h>
#include <liim/fixed_array.h>
#include <limits.h>

static constexpr decltype(auto) build_crc32_table(uint32_t polynomial) {
    FixedArray<uint32_t, UINT8_MAX + 1> table;
    for (uint16_t i = 0; i < table.size(); i++) {
        uint32_t value = i;
        for (int k = 0; k < CHAR_BIT; k++) {
            if (value & 1) {
                value = polynomial ^ (value >> 1);
            } else {
                value >>= 1;
            }
        }
        table[i] = value;
    }
    return table;
}

static constexpr auto crc32_table = build_crc32_table(0xEDB88320U);
static_assert(crc32_table[0] == 0);
static_assert(crc32_table[1] == 0x77073096);
static_assert(crc32_table[2] == 0xEE0E612C);
static_assert(crc32_table[3] == 0x990951BA);

extern "C" {
uint32_t compute_partial_crc32_checksum(const void* data, size_t num_bytes, uint32_t start) {
    uint32_t sum = ~start;
    for (size_t i = 0; i < num_bytes; i++) {
        uint8_t index = (sum ^ ((const uint8_t*) data)[i]) & 0xFF;
        sum = (sum >> 8) ^ crc32_table[index];
    }
    return ~sum;
}

uint32_t compute_crc32_checksum(const void* data, size_t num_bytes) {
    return compute_partial_crc32_checksum(data, num_bytes, CHECKSUM_CRC32_INIT);
}
}
