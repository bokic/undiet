#include "undiet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                             \
    do {                                                                        \
        if (!(cond)) {                                                          \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #cond);                                                     \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

static void write_header(uint8_t *buf, uint32_t packed_size, uint32_t unpacked_size, uint16_t crc) {
    memset(buf, 0, UNDIET_HEADER_SIZE);
    buf[0] = 0xb4;
    buf[1] = 0x4c;
    buf[4] = 0x9d;
    buf[5] = 0x89;
    buf[6] = 0x64;
    buf[7] = 0x6c;
    buf[9]  = (uint8_t)(((packed_size >> 16) & 0x0f) << 2);
    buf[10] = (uint8_t)(packed_size & 0xff);
    buf[11] = (uint8_t)((packed_size >> 8) & 0xff);
    buf[12] = (uint8_t)(crc & 0xff);
    buf[13] = (uint8_t)((crc >> 8) & 0xff);
    buf[14] = (uint8_t)(((unpacked_size >> 16) & 0x3f) << 2);
    buf[15] = (uint8_t)(unpacked_size & 0xff);
    buf[16] = (uint8_t)((unpacked_size >> 8) & 0xff);
}

static void test_rle_match_and_crc(void) {
    // "ABCABCABC" (9 bytes):
    // 3 literals ('A','B','C'), match (x1=0, x2=1, a1=1, a2=1, v=0xfd -> dist=3; len=6 -> 0,0,0,1),
    // STOP (x1=0, x2=0, v=0xff, a1=0, a2=0).
    // Control word: 0x0477 -> 0x77, 0x04
    const uint8_t payload[] = {0x77, 0x04, 'A', 'B', 'C', 0xfd, 0xff};
    uint32_t total = UNDIET_HEADER_SIZE + sizeof(payload);
    uint8_t *src = malloc(total);
    CHECK(src != NULL);
    write_header(src, sizeof(payload), 9, 0);
    memcpy(src + UNDIET_HEADER_SIZE, payload, sizeof(payload));

    uint16_t crc = undiet_calc_crc16(src, total);
    write_header(src, sizeof(payload), 9, crc);
    CHECK(undiet_isvalid(src, total));
    CHECK(undiet_get_compressed_size(src, total) == sizeof(payload));
    CHECK(undiet_get_uncompressed_size(src, total) == 9);
    CHECK(undiet_get_crc(src, total) == crc);

    uint8_t dst[9] = {0};
    int32_t n = undiet_unpack_ex(src, total, dst, sizeof(dst));
    CHECK(n == 9);
    CHECK(memcmp(dst, "ABCABCABC", 9) == 0);

    // Also test legacy undiet_unpack
    memset(dst, 0, sizeof(dst));
    CHECK(undiet_unpack(src, dst) == 9);
    CHECK(memcmp(dst, "ABCABCABC", 9) == 0);

    free(src);
}

static void test_eof_on_16bit_boundary(void) {
    // 12 literals ('A'..'L') + STOP (0,0,0,0) = exact 16 bits (0x0fff -> 0xff, 0x0f)
    const uint8_t payload[] = {
        0xff, 0x0f,
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        0xff
    };
    uint32_t total = UNDIET_HEADER_SIZE + sizeof(payload);
    uint8_t *src = malloc(total);
    CHECK(src != NULL);
    write_header(src, sizeof(payload), 12, 0);
    memcpy(src + UNDIET_HEADER_SIZE, payload, sizeof(payload));

    uint8_t dst[12] = {0};
    int32_t n = undiet_unpack_ex(src, total, dst, sizeof(dst));
    CHECK(n == 12);
    CHECK(memcmp(dst, "ABCDEFGHIJKL", 12) == 0);

    free(src);
}

static void test_error_bounds(void) {
    // 1. match_dist > dst_idx (match at offset 0 with dist=2)
    // x1=0, x2=0, a1=0, v=0xfe -> dist = 256 - 0xfe = 2, dst_idx = 0
    const uint8_t bad_dist[] = {0x00, 0x00, 0xfe};
    uint8_t src1[UNDIET_HEADER_SIZE + sizeof(bad_dist)];
    write_header(src1, sizeof(bad_dist), 2, 0);
    memcpy(src1 + UNDIET_HEADER_SIZE, bad_dist, sizeof(bad_dist));
    uint8_t dst[16] = {0};
    CHECK(undiet_unpack_ex(src1, sizeof(src1), dst, sizeof(dst)) == -1);

    // 2. Undersized dst buffer
    const uint8_t lit3[] = {0x07, 0x00, 'A', 'B', 'C'};
    uint8_t src2[UNDIET_HEADER_SIZE + sizeof(lit3)];
    write_header(src2, sizeof(lit3), 3, 0);
    memcpy(src2 + UNDIET_HEADER_SIZE, lit3, sizeof(lit3));
    CHECK(undiet_unpack_ex(src2, sizeof(src2), dst, 2) == -1);

    // 3. Truncated src (no STOP code)
    CHECK(undiet_unpack_ex(src2, sizeof(src2), dst, sizeof(dst)) == -1);
}

int main(void) {
    test_rle_match_and_crc();
    test_eof_on_16bit_boundary();
    test_error_bounds();
    printf("All undiet tests passed.\n");
    return 0;
}
