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

   *dl *= 2; if (*cf) *dl |= 0x01;                                        // rcl     dl, 1

    if (*src_offset >= 0x2006) {                                          // cmp     $0x2006, %r11
        *src_offset -= 0x2000;                                            // sub     $0x2000, %r11 # old_si
        *src_seg    += 0x2000;                                            // add     $0x2000, %r14 # old_ds
    }

    if (*dst_offset >= 0xc000) {                                          // cmp     $0xc000, %r12 # old_di
        *dst_offset -= 0x8000;                                            // sub     $0x8000, %r12 # old_di
        *dst_seg    += 0x8000;                                            // add     $0x8000, %r13 # old_es
    }

    *ax =  *(uint16_t *)(src + *src_seg + *src_offset); *src_offset += 2; // lodsw

    tmp = *ax; *ax = *bp; *bp = tmp;                                      // xchg    ax, bp
    *cf = *dl & 0x01; *dl = *dl / 2;                                      // shr     $1, %dl
    *dl = 0x10;                                                           // mov     dl, 10h
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

    // loc_115C5
    d.l = 0x10;                                                                                               // mov     dl, 10h
    a.x = *(uint16_t *)(src + src_offset); src_offset += 2;                                                   // lodsw
    tmp = a.x; a.x = bp; bp = tmp;                                                                            // xchg    ax, bp
    c.x = 0;                                                                                                  // xor     cx, cx

    while(1) {
        while(1) { // ok
            // loc_115DC
            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl ; (do not touch CF)
            if (d.l == 0) {                                                                                   // jz      short loc_115CD
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
                                                                                                              // jmp     short loc_1459
            }

            // loc_115E2
            if (cf == 0) break;                                                                               // jnb     short loc_115E7 or CF == 0

            dst[dst_seg + dst_offset] = src[src_offset + src_seg]; dst_offset++; src_offset++;                // movsb
        }

        // loc_115E7
        cf = bp & 0x01; bp = bp / 2;                                                                          // shr     bp, 1
        d.l--;                                                                                                // dec     dl ; (do not touch CF)
        if (d.l == 0) {                                                                                       // jz      short loc_115D2
            undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);             // call    sub_1368
        }

        // loc_115ED:
        a.l = src[src_offset + src_seg]; src_offset++;                                                        // lodsb
        a.h = 0xff;                                                                                           // mov     ah, 0FFh
        tmp = a.x; a.x = b.x; b.x = tmp;                                                                      // xchg    ax, bx

        // loc_115F1
        if (cf == 0) { // left branch                                                                         // jb      loc_1163A

            // loc_115F3
            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl ; (do not touch CF)

            // loc_115F7
            if (d.l == 0) {                                                                                   // jz      short loc_115D7
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
            }

            // loc_115F9
            if (cf == 0) {                                                                                    // jb      short loc_11609
                if (b.l == b.h) {                                                                             // cmp     bl, bh
                    return dst_seg + dst_offset;
                }
            } else {
                c.l = 3;                                                                                      // mov     cl, 3

                // loc_1160B
                do {
                    cf = bp & 0x01; bp = bp / 2;                                                              // shr     bp, 1
                    d.l--;                                                                                    // dec     dl ; (do not touch CF)

                    if (d.l == 0) {                                                                           // jz      short loc_115D7
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf); // call    sub_1368
                    }

                    b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;                                           // rcl     bh, 1
                } while (--c.x > 0);                                                                          // loop    loc_1160B

                b.h--;                                                                                        // dec     bh
            }

            // loc_115FF
            c.l = 2;                                                                                          // mov     cl, 2

        } else { // right branch
            // loc_1163A:
            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl ; (do not touch CF)

            if (d.l == 0) {                                                                                   // jz      short loc_1161C
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
            }

            // loc_11640:
            b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;                                                   // rcl     bh, 1
            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl ; (do not touch CF)

            if (d.l == 0) {                                                                                   // jz      short loc_150C
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
            }

            // loc_11648
            if (cf == 0) {                                                                                    // jb      short loc_1564
                d.h = 2;                                                                                      // mov     dh, 2
                c.l = 3;                                                                                      // mov     cl, 3

                do {                                                                                          // loc_154C:
                    cf = bp & 0x01; bp = bp / 2;                                                              // shr     bp, 1
                    d.l--;                                                                                    // dec     dl ; (do not touch CF)
                    if (d.l == 0) {                                                                           // jz      short loc_151A
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf); // call    sub_1368
                    }

                    if (cf != 0) break;                                                                       // jb      short loc_1562

                    cf = bp & 0x01; bp = bp / 2;                                                              // shr     bp, 1
                    d.l--;                                                                                    // dec     dl ; (do not touch CF)

                    if (d.l == 0) {                                                                           // jz      short loc_151A
                        undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf); // call    sub_1368
                    }

                    b.h *= 2; if (cf) b.h |= 0x01; cf = b.h & 0x80;                                           // rcl     bh, 1
                    d.h *= 2;                                                                                 // add     dh, dh

                } while (--c.x > 0);                                                                          // loop    loc_154C

                // loc_1562:
                b.h -= d.h;                                                                                   // sub     bh, dh
            }

            // loc_1564:
            d.h = 2;                                                                                          // mov     dh, 2
            c.l = 4;                                                                                          // mov     cl, 4

            do {
                d.h++;                                                                                        // inc     dh
                cf = bp & 0x01; bp = bp / 2;                                                                  // shr     bp, 1
                d.l--;                                                                                        // dec     dl

                if (d.l == 0) {                                                                               // jz      short loc_151A
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);     // call    sub_1368
                }

                // loc_1571:
                if (cf != 0) {                                                                                // jb      short loc_158A
                    c.l = d.h;                                                                                // mov     cl, dh
                    goto loc_11601;
                }

            } while (--c.x > 0);                                                                              // loop    loc_1568

            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl

            if (d.l == 0) {                                                                                   // jz      short loc_15DF
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
            }

            // loc_1167A
            if (cf != 0) {                                                                                    // jb      short loc_116A7
                d.h++;                                                                                        // inc     dh
                cf = bp & 0x01; bp = bp / 2;                                                                  // shr     bp, 1
                d.l--;                                                                                        // dec     dl

                if (d.l == 0) {                                                                               // jz      short loc_151A
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);     // call    sub_1368
                }

                // loc_11684
                if (cf) d.h++;                                                                                // adc     dh, 0
                c.l = d.h;                                                                                    // mov     cl, dh

                goto loc_11601;                                                                               // jmp     loc_14C0
            }

            // loc_1168C
            cf = bp & 0x01; bp = bp / 2;                                                                      // shr     bp, 1
            d.l--;                                                                                            // dec     dl

            if (d.l == 0) {                                                                                   // jz      short loc_151A
                undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);         // call    sub_1368
            }

            if (cf != 0) {
                // loc_116A7
                a.l = src[src_offset + src_seg]; src_offset++;                                                // lodsb
                c.l = a.l;                                                                                    // mov     cl, al
                c.x += 0x11;                                                                                  // add     cx, 11h
                goto loc_11601;
            }

            c.l = 3;                                                                                          // mov     cl, 3
            d.h = 0;                                                                                          // mov     dh, 0

            // loc_11698
            do {
                cf = bp & 0x01; bp = bp / 2;                                                                  // shr     bp, 1
                d.l--;                                                                                        // dec     dl

                if (d.l == 0) {                                                                               // jz      short loc_151A
                    undiet_next(src, &a.x, &bp, &d.l, &src_offset, &dst_offset, &dst_seg, &src_seg, &cf);     // call    sub_1368
                }

                // loc_1169E
                d.h *= 2; if (cf) d.h |= 0x01;                                                                // rcl     dh, 1
            } while (--c.x > 0);                                                                              // loop    loc_11698

            d.h += 9;                                                                                         // add     dh, 9
            c.l = d.h;                                                                                        // mov     cl, dh
        }

loc_11601:
        do {
            a.l = dst[dst_seg + ((b.x + dst_offset) & 0xffff)];                                               // mov  al,es:[bx+di]
            dst[dst_seg + dst_offset] = a.l; dst_offset = (dst_offset + 1) & 0xffff;                          // stosb
        } while(--c.x > 0);
    }
}

bool undiet_isvalid(uint8_t *src, uint32_t size)
{
    return ((size > 12)&&(size <= 0x40000)&&(src[0] == 0xb4)&&(src[1] == 0x4c)&&(src[4] == 0x9d)&&(src[5] == 0x89)&&(src[6] == 0x64)&&(src[7] == 0x6c));
}
