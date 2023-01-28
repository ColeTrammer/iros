#include <ext/checksum.h>

constexpr uint16_t adler32_base = 65521U;

uint32_t compute_partial_adler32_checksum(const void *data, size_t num_bytes, uint32_t adler) {
    uint32_t s1 = (adler >> 0) & 0xFFFFU;
    uint32_t s2 = (adler >> 16) & 0xFFFFU;

    auto *bytes = (const uint8_t *) data;
    for (size_t i = 0; i < num_bytes; i++) {
        s1 = (s1 + bytes[i]) % adler32_base;
        s2 = (s2 + s1) % adler32_base;
    }

    return (s2 << 16U) | s1;
}

uint32_t compute_adler32_checksum(const void *data, size_t num_bytes) {
    return compute_partial_adler32_checksum(data, num_bytes, CHECKSUM_ADLER32_INIT);
}
