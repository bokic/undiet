#ifndef __UNDIET_H
#define __UNDIET_H

#include <stdbool.h>
#include <stdint.h>

#define        UNDIET_HEADER_SIZE 17
const uint32_t UNDIET_MAX_PACKED_FILESIZE   =  0xfffff;
const uint32_t UNDIET_MAX_UNPACKED_FILESIZE = 0x3fffff;

int32_t undiet_unpack(const uint8_t src[], uint8_t dst[]);
bool undiet_isvalid(const uint8_t src[], uint32_t size);
uint32_t undiet_get_uncompressed_size(const uint8_t src[], uint32_t size);
uint32_t undiet_get_compressed_size(const uint8_t src[], uint32_t size);
uint16_t undiet_calc_crc16(const uint8_t src[], uint32_t size);

#endif // __UNDIET_H
