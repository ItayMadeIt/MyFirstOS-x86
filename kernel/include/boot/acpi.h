#ifndef __ACPI_H__
#define __ACPI_H__

#include <core/defs.h>

typedef struct acpi_sdt_header 
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;


typedef struct generic_address_struct
{
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
}  __attribute__((packed))  generic_address_struct_t;


enum acpi_timer_flags {
    ACPI_TIMER_ACTIVE = 1 << 0, // On = Active, Off = Cannot be used
    ACPI_TIMER_IO     = 1 << 1, // On = IO, Off = Not IO 
    ACPI_TIMER_MMIO   = 1 << 2, // On = MMIO, Off = Not MMIO
    ACPI_TIMER_24     = 1 << 3, // On = 24 bits, Off = 32 bits
};

typedef struct acpi_timer {
    uint8_t flags; // acpi_timer_flags
    union {
        generic_address_struct_t mem_region;
        uint32_t io_port;
    };
} acpi_timer_t;

extern acpi_timer_t acpi_timer;

bool valid_checksum(acpi_sdt_header_t* table_header);

uint32_t get_acpi_time();

void setup_acpi();

#endif // __ACPI_H__