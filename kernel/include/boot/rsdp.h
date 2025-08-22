#ifndef __RSDP_H__
#define __RSDP_H__

#include <core/defs.h>


// RSDP/XSDP - Root/eXtended System Descriptor Table
// Info about where the Rsdt or Xsdt (the pointer points to those structs) are located
// 

typedef struct rsdp 
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed)) rsdp_t;


typedef struct xsdp
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;      // deprecated since version 2.0

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__ ((packed)) xsdp_t;

// can be cast to xsdp
rsdp_t* gather_rdsp();

#endif // __RSDP_H__