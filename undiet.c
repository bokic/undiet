#include "undiet.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const uint16_t crc16_table[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
};

typedef struct {
    const uint8_t *src;
    uint32_t offset;
    uint16_t buffer;
    uint8_t bits_left;
    uint32_t dst_idx;
} BitStream;

static inline void bitstream_refill(BitStream *bs) {
    if (bs->bits_left == 0) {
        // Read next 16-bit word (LSB first)
        bs->buffer = (uint16_t)bs->src[bs->offset] | ((uint16_t)bs->src[bs->offset + 1] << 8);
        bs->offset += 2;
        bs->bits_left = 16;
    }
}

static inline bool get_bit(BitStream *bs) {
    if (bs->bits_left == 0) {
        bitstream_refill(bs);
    }
    bool bit = bs->buffer & 1;
    bs->buffer >>= 1;
    bs->bits_left--;
    if (bs->bits_left == 0) {
        bitstream_refill(bs);
    }
    return bit;
}

int32_t undiet_unpack(const uint8_t src[], uint8_t dst[]) {
    if (!src || !dst) return 0;

    BitStream bs = {
        .src = src + UNDIET_HEADER_SIZE,
        .offset = 0,
        .buffer = 0,
        .bits_left = 0,
        .dst_idx = 0
    };

    bitstream_refill(&bs);

    while (1) {
        if (get_bit(&bs)) { // x1
            // Literal
            dst[bs.dst_idx++] = bs.src[bs.offset++];
        } else {
            // Match
            bool x2 = get_bit(&bs);
            uint8_t v = bs.src[bs.offset++]; // dist_lo (v in deark)
            uint32_t matchpos = 0;
            uint32_t matchlen = 0;

            if (!x2) { // 00... -> 2-byte match or special code
                if (get_bit(&bs)) { // a1 == 1: "long" two-byte match
                    matchlen = 2;
                    uint32_t a2 = get_bit(&bs) ? 1 : 0;
                    uint32_t a3 = get_bit(&bs) ? 1 : 0;
                    uint32_t a4 = get_bit(&bs) ? 1 : 0;
                    matchpos = 2303 - (1024 * a2 + 512 * a3 + 256 * a4 + (uint32_t)v);
                } else if (v != 0xff) { // a1 == 0, v != 0xff: "short" two-byte match
                    matchlen = 2;
                    matchpos = 255 - (uint32_t)v;
                } else { // v == 0xff: special code
                    if (!get_bit(&bs)) { // a2 == 0: STOP
                        return (int32_t)bs.dst_idx;
                    }
                    // a2 == 1: segment refresh (ignoring for now as it's for EXE)
                    continue;
                }
            } else { // 01... -> 3 or more byte match
                uint32_t a1 = get_bit(&bs) ? 1 : 0;
                uint32_t a2 = get_bit(&bs) ? 1 : 0;

                if (a2) {
                    matchpos = 511 - (256 * a1 + (uint32_t)v);
                } else {
                    if (get_bit(&bs)) { // a3 == 1
                        matchpos = 1023 - (256 * a1 + (uint32_t)v);
                    } else {
                        uint32_t a4 = get_bit(&bs) ? 1 : 0;
                        if (get_bit(&bs)) { // a5 == 1
                            matchpos = 2047 - (512 * a1 + 256 * a4 + (uint32_t)v);
                        } else {
                            uint32_t a6 = get_bit(&bs) ? 1 : 0;
                            if (get_bit(&bs)) { // a7 == 1
                                matchpos = 4095 - (1024 * a1 + 512 * a4 + 256 * a6 + (uint32_t)v);
                            } else {
                                uint32_t a8 = get_bit(&bs) ? 1 : 0;
                                matchpos = 8191 - (2048 * a1 + 1024 * a4 + 512 * a6 + 256 * a8 + (uint32_t)v);
                            }
                        }
                    }
                }

                // read_matchlen logic
                uint32_t nbits_read = 0;
                while (1) {
                    bool x = get_bit(&bs);
                    nbits_read++;
                    if (x) {
                        matchlen = 2 + nbits_read;
                        break;
                    }
                    if (nbits_read >= 4) {
                        // All 4 bits were 0
                        uint32_t x1_len = get_bit(&bs) ? 1 : 0;
                        uint32_t x2_len = get_bit(&bs) ? 1 : 0;
                        if (x1_len == 1) { // 7-8
                            matchlen = 7 + x2_len;
                        } else if (x2_len == 0) { // 9-16
                            uint32_t x3 = get_bit(&bs) ? 1 : 0;
                            uint32_t x4 = get_bit(&bs) ? 1 : 0;
                            uint32_t x5 = get_bit(&bs) ? 1 : 0;
                            matchlen = 9 + 4 * x3 + 2 * x4 + x5;
                        } else { // 17-272
                            matchlen = 17 + (uint32_t)bs.src[bs.offset++];
                        }
                        break;
                    }
                }
            }

            // Copy match
            uint32_t match_dist = matchpos + 1;
            
            if (match_dist >= matchlen) {
                memcpy(&dst[bs.dst_idx], &dst[bs.dst_idx - match_dist], matchlen);
                bs.dst_idx += matchlen;
            } else {
                do {
                    dst[bs.dst_idx] = dst[bs.dst_idx - match_dist];
                    bs.dst_idx++;
                } while (--matchlen > 0);
            }
        }
    }
}

bool undiet_isvalid(const uint8_t src[], uint32_t size) {
    if (size <= 16 || size > UNDIET_MAX_PACKED_FILESIZE) return false;
    return (src[0] == 0xb4) && (src[1] == 0x4c) && 
           (src[4] == 0x9d) && (src[5] == 0x89) && (src[6] == 0x64) && (src[7] == 0x6c);
}

uint32_t undiet_get_uncompressed_size(const uint8_t src[], uint32_t size) {
    if (!src || size <= UNDIET_HEADER_SIZE) return 0;
    return ((((uint32_t)src[14] >> 2) & 0x3f) << 16) | ((uint32_t)src[16] << 8) | src[15];
}

uint32_t undiet_get_compressed_size(const uint8_t src[], uint32_t size) {
    if (!src || size <= UNDIET_HEADER_SIZE) return 0;
    return ((((uint32_t)src[9] >> 2) & 0x0f) << 16) | ((uint32_t)src[11] << 8) | src[10];
}

uint16_t undiet_get_crc(const uint8_t src[], uint32_t size) {
    if (!src || size <= UNDIET_HEADER_SIZE) return 0;
    return ((uint16_t)src[13] << 8) | src[12];
}

uint16_t undiet_calc_crc16(const uint8_t src[], uint32_t size) {
    if (!src || size < UNDIET_HEADER_SIZE || size > UNDIET_MAX_PACKED_FILESIZE) return 0;

    const uint8_t *data = src + UNDIET_HEADER_SIZE;
    uint32_t len = size - UNDIET_HEADER_SIZE;
    uint16_t crc = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xff];
    }

    return crc;
}
