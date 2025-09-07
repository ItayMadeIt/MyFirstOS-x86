#ifndef __FADT_H__
#define __FADT_H__

#include <core/defs.h>
#include <firmware/acpi.h>
#include <firmware/rsdt.h>

// FADT - Fixed ACPI Description Table
// Info about fixed registers and other important fixed data

typedef struct fadt
{
    acpi_sdt_header_t header;

    uint32_t firmware_ctrl;
    uint32_t dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  reserved;

    uint8_t  preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t  acpi_enable;
    uint8_t  acpi_disable;
    uint8_t  S4BIOS_req;
    uint8_t  pstate_control;
    uint32_t PM1a_event_block;
    uint32_t PM1b_event_block;
    uint32_t PM1a_control_block;
    uint32_t PM1b_control_block;
    uint32_t PM2_control_block;
    uint32_t PM_timer_block;
    uint32_t GPE0_block;
    uint32_t GPE1_block;
    uint8_t  PM1_event_length;
    uint8_t  PM1_control_length;
    uint8_t  PM2_control_length;
    uint8_t  PM_timer_length;
    uint8_t  GPE0_length;
    uint8_t  GPE1_length;
    uint8_t  GPE1_base;
    uint8_t  C_state_control;
    uint16_t worst_C2_latency;
    uint16_t worst_C3_Latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alarm;
    uint8_t  month_alarm;
    uint8_t  century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t boot_architecture_flags;

    uint8_t  reserved2;
    uint32_t flags;

    // 12 byte structure; see below for details
    generic_address_struct_t reset_reg;

    uint8_t  reset_value;
    uint8_t  reserved3[3];
  
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_firmware_control;
    uint64_t                X_dsdt;

    generic_address_struct_t X_PM1a_event_block;
    generic_address_struct_t X_PM1b_event_block;
    generic_address_struct_t X_PM1a_control_block;
    generic_address_struct_t X_PM1b_control_block;
    generic_address_struct_t X_PM2_control_block;
    generic_address_struct_t X_PM_timer_block;
    generic_address_struct_t X_GPE0_block;
    generic_address_struct_t X_GPE1_block;
} __attribute__((packed)) fadt_t;
#endif // __FADT_H__

fadt_t* find_fadt_by_rsdt(rsdt_t* rsdt);
fadt_t* find_fadt_by_xsdt(xsdt_t* xsdt);