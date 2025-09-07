#ifndef __RSDT_H__
#define __RSDT_H__

#include <stdint.h>
#include <firmware/acpi.h>

// RDST/XDST - Root/eXtended system description table
// This table contains pointers to all the other System Description Tables

typedef struct rsdt 
{
    acpi_sdt_header_t header;
    uint32_t pointer_sdts[]; // count = (header.length - sizeof(header)) / 4
} rsdt_t;


typedef struct xsdt 
{
    acpi_sdt_header_t header;
    uint64_t pointer_sdts[]; // count (header.length - sizeof(header)) / 8
} __attribute__((packed)) xsdt_t;
#endif // __RSDT_H__