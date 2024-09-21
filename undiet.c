#include "undiet.h"

#include <stdbool.h>
#include <stdint.h>


typedef union {
   uint16_t x;
   struct {
       uint8_t l;
       uint8_t h;
   };
} reg;

static inline void undiet_next(const uint8_t src[], uint16_t *ax, uint16_t *bp, uint8_t *dl, uint32_t *src_offset, uint32_t *dst_offset, uint32_t *dst_seg, bool *cf)
{
    uint16_t tmp = 0;

   *dl *= 2; if (*cf) *dl |= 0x01;


    if (*dst_offset >= 0xc000) {
        *dst_offset -= 0x8000;
        *dst_seg    += 0x8000;
    }

    *ax =  *(uint16_t *)(src + *src_offset); *src_offset += 2; // lodsw

    tmp = *ax; *ax = *bp; *bp = tmp;
    *cf = *dl & 0x01; *dl = *dl / 2;
    *dl = 0x10;
}

int32_t undiet_unpack(const uint8_t src[], uint8_t dst[])
{
    reg a = {0};
    reg b = {5};
    reg c = {0};
    reg d = {0};

    uint32_t src_offset = 0;
    uint32_t dst_seg = 0;
    uint32_t dst_offset = 0;

    uint16_t bp = 0x0a;
    bool cf = 0;

    uint16_t tmp = 0;

    src += UNDIET_HEADER_SIZE;

    d.l = 0x10;
    a.x = *(uint16_t *)(src + src_offset); src_offset += 2;
    tmp = a.x; a.x = bp; bp = tmp;
    c.x = 0;

    while(1) {
        while(1) {
            cf = bp & 0x01; bp = bp / 2;
            d.l--;
            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            if (cf == 0) break;

            dst[dst_seg + dst_offset] = src[src_offset]; dst_offset++; src_offset++;                          // movsb
        }

        cf = bp & 0x01; bp = bp / 2;
        d.l--;
        if (d.l == 0) {
            undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                       // call    sub_1368
        }

        a.l = src[src_offset]; src_offset++;                                                                  // lodsb
        a.h = 0xff;
        tmp = a.x; a.x = b.x; b.x = tmp;

        if (cf == 0) {

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            if (cf == 0) {
                if (b.l == b.h) {
                    return dst_seg + dst_offset;
                }
            } else {
                c.l = 3;

                do {
                    cf = bp & 0x01; bp = bp / 2;
                    d.l--;

                    if (d.l == 0) {
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);           // call    sub_1368
                    }

                    b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;
                } while (--c.x > 0);

                b.h--;
            }

            c.l = 2;

        } else {
            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;
            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            if (cf == 0) {
                d.h = 2;
                c.l = 3;

                do {
                    cf = bp & 0x01; bp = bp / 2;
                    d.l--;
                    if (d.l == 0) {
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);           // call    sub_1368
                    }

                    if (cf != 0) break;

                    cf = bp & 0x01; bp = bp / 2;
                    d.l--;

                    if (d.l == 0) {
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);           // call    sub_1368
                    }

                    b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;
                    d.h *= 2;

                } while (--c.x > 0);

                b.h -= d.h;
            }

            d.h = 2;
            c.l = 4;

            do {
                d.h++;
                cf = bp & 0x01; bp = bp / 2;
                d.l--;

                if (d.l == 0) {
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);               // call    sub_1368
                }

                if (cf != 0) {
                    c.l = d.h;
                    goto loc_11601;
                }

            } while (--c.x > 0);

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            if (cf != 0) {
                d.h++;
                cf = bp & 0x01; bp = bp / 2;
                d.l--;

                if (d.l == 0) {
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);               // call    sub_1368
                }

                if (cf) d.h++;
                c.l = d.h;

                goto loc_11601;
            }

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);                   // call    sub_1368
            }

            if (cf != 0) {
                a.l = src[src_offset]; src_offset++;                                                          // lodsb
                c.l = a.l;
                c.x += 0x11;
                goto loc_11601;
            }

            c.l = 3;
            d.h = 0;

            do {
                cf = bp & 0x01; bp = bp / 2;
                d.l--;

                if (d.l == 0) {
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &cf);               // call    sub_1368
                }

                d.h *= 2; if (cf) d.h |= 0x01;
            } while (--c.x > 0);

            d.h += 9;
            c.l = d.h;
        }

loc_11601:
        do {
            a.l = dst[dst_seg + ((b.x + dst_offset) & 0xffff)];
            dst[dst_seg + dst_offset] = a.l; dst_offset = (dst_offset + 1) & 0xffff;
        } while(--c.x > 0);
    }

    return dst_seg + dst_offset + 1;
}

bool undiet_isvalid(const uint8_t src[], uint32_t size)
{
    return ((size > 16)&&(size <= UNDIET_MAX_PACKED_FILESIZE)&&(src[0] == 0xb4)&&(src[1] == 0x4c)&&(src[4] == 0x9d)&&(src[5] == 0x89)&&(src[6] == 0x64)&&(src[7] == 0x6c));
}

uint32_t undiet_get_uncompressed_size(const uint8_t src[], uint32_t size)
{
    if(!src)
        return 0;

    if (size <= UNDIET_HEADER_SIZE)
        return 0;

    return ((((uint32_t)src[14] >> 2) & 0x3f) << 16) | ((uint32_t)src[16] << 8) | src[15];
}

uint32_t undiet_get_compressed_size(const uint8_t src[], uint32_t size)
{
    if(!src)
        return 0;

    if (size <= UNDIET_HEADER_SIZE)
        return 0;

    return ((((uint32_t)src[9] >> 2) & 0x0f) << 16) | ((uint32_t)src[11] << 8) | src[10];
}

uint16_t undiet_get_crc(const uint8_t src[], uint32_t size)
{
    if(!src)
        return 0;

    if (size <= UNDIET_HEADER_SIZE)
        return 0;

    return ((uint16_t)src[13] << 8) | src[12];
}

uint16_t undiet_calc_crc16(const uint8_t src[], uint32_t size)
{
    const uint16_t CRC16 = 0x8005;

    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    if(!src)
        return 0;

    if ((size < UNDIET_HEADER_SIZE)||(size > UNDIET_MAX_PACKED_FILESIZE))
        return 0;

    src += UNDIET_HEADER_SIZE;
    size -= UNDIET_HEADER_SIZE;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*src >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            src++;
            size--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}
