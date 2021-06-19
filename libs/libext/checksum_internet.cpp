#include <arpa/inet.h>
#include <ext/checksum.h>

extern "C" {
uint16_t compute_partial_internet_checksum(const void *packet, size_t num_bytes, uint16_t start) {
    const uint16_t *raw_data = (const uint16_t *) packet;
    uint32_t sum = ~start & 0xFFFF;

    // Sum everything 16 bits at a time
    for (size_t i = 0; i < num_bytes / sizeof(uint16_t); i++) {
        // Prevent overflow
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        sum += ntohs(raw_data[i]);
    }

    // Handle trailing byte
    if (num_bytes % 2 == 1) {
        sum += ((uint16_t)((const uint8_t *) packet)[num_bytes - 1]) << 8;
    }

    // 1's complement the carry
    while (sum & ~0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Invert the sum for storage
    return (uint16_t)(~sum & 0xFFFF);
}

uint16_t compute_internet_checksum(const void *packet, size_t num_bytes) {
    return compute_partial_internet_checksum(packet, num_bytes, ~0U & 0xFFFF);
}
}
