#ifndef __RSDT_H__
#define __RSDT_H__

#include "core/num_defs.h"
#include <firmware/acpi/acpi.h>

// RDST/XDST - Root/eXtended system description table
// This table contains pointers to all the other System Description Tables

typedef struct rsdt 
{
    acpi_sdt_header_t header;
    u32 pointer_sdts[]; // count = (header.length - sizeof(header)) / 4
} rsdt_t;


typedef struct xsdt 
{
    acpi_sdt_header_t header;
    u64 pointer_sdts[]; // count (header.length - sizeof(header)) / 8
} __attribute__((packed)) xsdt_t;
#endif // __RSDT_H__