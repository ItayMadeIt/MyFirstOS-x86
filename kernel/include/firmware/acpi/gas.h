#ifndef __GAS_H__
#define __GAS_H__

#include <stdint.h>

typedef struct generic_address_struct
{
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
}  __attribute__((packed))  generic_address_struct_t;

uint32_t gas_unit_size(uint8_t access_size, uint8_t bit_width) ;

#endif // __GAS_H__