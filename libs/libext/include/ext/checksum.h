#ifndef _EXT_CHECKSUM_H
#define _EXT_CHECKSUM_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint32_t compute_partial_crc32_checksum(void *data, size_t num_bytes, uint32_t crc);
uint32_t compute_crc32_checksum(void *data, size_t num_bytes);

uint16_t compute_partial_internet_checksum(void *packet, size_t num_bytes, uint16_t start);
uint16_t compute_internet_checksum(void *data, size_t bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EXT_CHECKSUM_H */
