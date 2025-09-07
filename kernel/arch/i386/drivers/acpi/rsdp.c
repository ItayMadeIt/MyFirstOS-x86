#include <core/defs.h>
#include <firmware/rsdp.h>
#include <string.h>

#define EBDA_MEM 0x40E

#define RSDP_SIGNATURE "RSD PTR "
#define RSDP_SIGNATURE_LEN 8
#define RDSP_ALIGNMENT 16

static bool valid_rsdp_checksum(rsdp_t* r) 
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < 20; i++)
    { 
        sum += ((uint8_t*)r)[i];
    }

    if (sum) 
        return false;

    if (r->revision >= 2) 
    {
        sum = 0;
        for (unsigned i=0; i < ((xsdp_t*)r)->length; i++) 
        {
            sum += ((uint8_t*)r)[i];
        }

        if (sum) 
            return false;
    }

    return true;
}


rsdp_t* gather_rdsp()
{
    uint32_t ebda_ptr = (*(uint16_t*)0x040E) << 4;
    for (uint32_t i = 0; i < STOR_1Kib; i+=RDSP_ALIGNMENT)
    {
        rsdp_t* rsdp = (rsdp_t*)(ebda_ptr + i) ;
        if (memcmp(rsdp->signature, RSDP_SIGNATURE, RSDP_SIGNATURE_LEN) == 0
            && valid_rsdp_checksum(rsdp))
        {
            return rsdp;
        }
    }

    void* begin_region = (void*)0x000E0000;
    void* end_region   = (void*)0x000FFFFF;

    uint32_t iterator = (uint32_t)begin_region;

    while (iterator <= (uint32_t)end_region)
    {
        rsdp_t* rsdp = (rsdp_t*)iterator;
        if (memcmp(rsdp->signature, RSDP_SIGNATURE, RSDP_SIGNATURE_LEN) == 0)
        {
            return rsdp;
        }

        iterator+=RDSP_ALIGNMENT;
    }

    return NULL;
}