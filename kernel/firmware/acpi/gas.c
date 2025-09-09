#include <firmware/acpi/gas.h>

uint32_t gas_unit_size(uint8_t access_size, uint8_t bit_width) 
{
    if (access_size >= 1 && access_size <= 4) 
    {
        static const uint32_t sz[5] = {0,1,2,4,8};
        return sz[access_size];
    }

    return (bit_width <= 8) ? 1 : 
            (bit_width <= 16) ? 2 : 
            (bit_width <= 32) ? 4 : 8;
}
