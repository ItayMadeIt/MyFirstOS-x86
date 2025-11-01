#ifndef __RSDP_H__
#define __RSDP_H__

#include <core/defs.h>


// RSDP/XSDP - Root/eXtended System Descriptor Table
// Info about where the Rsdt or Xsdt (the pointer points to those structs) are located
// 

typedef struct rsdp 
{
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
} __attribute__ ((packed)) rsdp_t;


typedef struct xsdp
{
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;      // deprecated since version 2.0

    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    u8 reserved[3];
} __attribute__ ((packed)) xsdp_t;

// can be cast to xsdp
rsdp_t* gather_rdsp();

#endif // __RSDP_H__