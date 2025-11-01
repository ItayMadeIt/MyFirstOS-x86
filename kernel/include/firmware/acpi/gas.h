#ifndef __GAS_H__
#define __GAS_H__

#include "core/num_defs.h"
typedef struct generic_address_struct
{
    u8 address_space;
    u8 bit_width;
    u8 bit_offset;
    u8 access_size;
    u64 address;
}  __attribute__((packed))  generic_address_struct_t;

u32 gas_unit_size(u8 access_size, u8 bit_width) ;

#endif // __GAS_H__