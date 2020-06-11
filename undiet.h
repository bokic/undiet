#ifndef __UNDIET_H
#define __UNDIET_H

#include <stdbool.h>
#include <stdint.h>


int32_t undiet_unpack(uint8_t *src, uint8_t *dst);
bool undiet_isvalid(uint8_t *src, uint32_t size);

#endif // __UNDIET_H
