#ifndef __ACPI_H__
#define __ACPI_H__

#include <core/defs.h>

typedef struct acpi_sdt_header 
{
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;


enum acpi_timer_flags {
    ACPI_TIMER_ACTIVE = 1 << 0, // On = Active, Off = Cannot be used
    ACPI_TIMER_IO     = 1 << 1, // On = IO, Off = Not IO 
    ACPI_TIMER_MMIO   = 1 << 2, // On = MMIO, Off = Not MMIO
    ACPI_TIMER_24     = 1 << 3, // On = 24 bits, Off = 32 bits
};

struct acpi_timer;
typedef struct acpi_timer acpi_timer_t;

bool valid_checksum(acpi_sdt_header_t* table_header);

u64 get_acpi_time();

void init_acpi();

#endif // __ACPI_H__