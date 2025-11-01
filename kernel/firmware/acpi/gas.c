#include <firmware/acpi/gas.h>

u32 gas_unit_size(u8 access_size, u8 bit_width) 
{
    if (access_size >= 1 && access_size <= 4) 
    {
        static const u32 sz[5] = {0,1,2,4,8};
        return sz[access_size];
    }

    return (bit_width <= 8) ? 1 : 
            (bit_width <= 16) ? 2 : 
            (bit_width <= 32) ? 4 : 8;
}
