#include "undiet.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


typedef union {
   uint16_t x;
   struct {
       uint8_t l;
       uint8_t h;
   };
} reg;

static void undiet_next(uint8_t *src, uint16_t *ax, uint16_t *bp, uint8_t *dl, uint32_t *src_offset, uint32_t *dst_offset, uint32_t *dst_seg, uint32_t *src_seg, bool *cf)
{
    uint16_t tmp = 0;

   *dl *= 2; if (*cf) *dl |= 0x01;

    if (*src_offset >= 0x2006) {
        *src_offset -= 0x2000;
        *src_seg    += 0x2000;
    }

    if (*dst_offset >= 0xc000) {
        *dst_offset -= 0x8000;
        *dst_seg    += 0x8000;
    }

    *ax =  *(uint16_t *)(src + *src_seg + *src_offset); *src_offset += 2;

    tmp = *ax; *ax = *bp; *bp = tmp;
    *cf = *dl & 0x01; *dl = *dl / 2;
    *dl = 0x10;
}

int32_t undiet_unpack(uint8_t *src, uint8_t *dst)
{
    reg a = {0};
    reg b = {5};
    reg c = {0};
    reg d = {0};

    uint32_t src_seg = 0;
    uint32_t src_offset = 0;
    uint32_t dst_seg = 0;
    uint32_t dst_offset = 0;

    uint16_t bp = 0x0a;
    bool cf = 0;

    uint16_t tmp = 0;

    src += 0x11; // skip diet header

    d.l = 0x10;
    a.x = *(uint16_t *)(src + src_offset); src_offset += 2;
    tmp = a.x; a.x = bp; bp = tmp;
    c.x = 0;

    while(1) {
        while(1) {
            cf = bp & 0x01; bp = bp / 2;
            d.l--;
            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);

            }

            if (cf == 0) break;

            dst[dst_seg + dst_offset] = src[src_offset + src_seg]; dst_offset++; src_offset++;
        }

        cf = bp & 0x01; bp = bp / 2;
        d.l--;
        if (d.l == 0) {
            undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
        }

        a.l = src[src_offset + src_seg]; src_offset++;
        a.h = 0xff;
        tmp = a.x; a.x = b.x; b.x = tmp;

        if (cf == 0) {

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
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
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
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
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
            }

            b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;
            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
            }

            if (cf == 0) {
                d.h = 2;
                c.l = 3;

                do {
                    cf = bp & 0x01; bp = bp / 2;
                    d.l--;
                    if (d.l == 0) {
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
                    }

                    if (cf != 0) break;

                    cf = bp & 0x01; bp = bp / 2;
                    d.l--;

                    if (d.l == 0) {
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
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
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
                }

                if (cf != 0) {
                    c.l = d.h;
                    goto loc_11601;
                }

            } while (--c.x > 0);

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
            }

            if (cf != 0) {
                d.h++;
                cf = bp & 0x01; bp = bp / 2;
                d.l--;

                if (d.l == 0) {
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
                }

                if (cf) d.h++;
                c.l = d.h;

                goto loc_11601;
            }

            cf = bp & 0x01; bp = bp / 2;
            d.l--;

            if (d.l == 0) {
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
            }

            if (cf != 0) {
                a.l = src[src_offset + src_seg]; src_offset++;
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
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);
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
}

bool undiet_isvalid(uint8_t *src, uint32_t size)
{
    return ((size > 12)&&(size <= 0x40000)&&(src[0] == 0xb4)&&(src[1] == 0x4c)&&(src[4] == 0x9d)&&(src[5] == 0x89)&&(src[6] == 0x64)&&(src[7] == 0x6c));
}
