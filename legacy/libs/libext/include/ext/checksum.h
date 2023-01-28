#ifndef _EXT_CHECKSUM_H
#define _EXT_CHECKSUM_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHECKSUM_CRC32_INIT    ((uint32_t) 0)
#define CHECKSUM_ADLER32_INIT  ((uint32_t) 1)
#define CHECKSUM_INTERNET_INIT ((uint16_t)(~0U & 0xFFFFU))

uint32_t compute_partial_crc32_checksum(const void *data, size_t num_bytes, uint32_t crc);
uint32_t compute_crc32_checksum(const void *data, size_t num_bytes);

uint32_t compute_partial_adler32_checksum(const void *data, size_t num_bytes, uint32_t adler);
uint32_t compute_adler32_checksum(const void *data, size_t num_bytes);

uint16_t compute_partial_internet_checksum(const void *packet, size_t num_bytes, uint16_t start);
uint16_t compute_internet_checksum(const void *data, size_t bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EXT_CHECKSUM_H */
