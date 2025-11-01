#ifndef __FADT_H__
#define __FADT_H__

#include <core/defs.h>
#include <firmware/acpi/acpi.h>
#include <firmware/acpi/rsdt.h>
#include <firmware/acpi/gas.h>

// FADT - Fixed ACPI Description Table
// Info about fixed registers and other important fixed data

typedef struct __attribute__((packed))  fadt
{
    acpi_sdt_header_t header;

    u32 firmware_ctrl;
    u32 dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    u8  reserved;

    u8  preferred_power_management_profile;
    u16 sci_interrupt;
    u32 smi_command_port;
    u8  acpi_enable;
    u8  acpi_disable;
    u8  S4BIOS_req;
    u8  pstate_control;
    u32 PM1a_event_block;
    u32 PM1b_event_block;
    u32 PM1a_control_block;
    u32 PM1b_control_block;
    u32 PM2_control_block;
    u32 PM_timer_block;
    u32 GPE0_block;
    u32 GPE1_block;
    u8  PM1_event_length;
    u8  PM1_control_length;
    u8  PM2_control_length;
    u8  PM_timer_length;
    u8  GPE0_length;
    u8  GPE1_length;
    u8  GPE1_base;
    u8  C_state_control;
    u16 worst_C2_latency;
    u16 worst_C3_Latency;
    u16 flush_size;
    u16 flush_stride;
    u8  duty_offset;
    u8  duty_width;
    u8  day_alarm;
    u8  month_alarm;
    u8  century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    u16 boot_architecture_flags;

    u8  reserved2;
    u32 flags;

    // 12 byte structure; see below for details
    generic_address_struct_t reset_reg;

    u8  reset_value;
    u8  reserved3[3];
  
    // 64bit pointers - Available on ACPI 2.0+
    u64                X_firmware_control;
    u64                X_dsdt;

    generic_address_struct_t X_PM1a_event_block;
    generic_address_struct_t X_PM1b_event_block;
    generic_address_struct_t X_PM1a_control_block;
    generic_address_struct_t X_PM1b_control_block;
    generic_address_struct_t X_PM2_control_block;
    generic_address_struct_t X_PM_timer_block;
    generic_address_struct_t X_GPE0_block;
    generic_address_struct_t X_GPE1_block;
} fadt_t;

fadt_t* find_fadt_by_rsdt(rsdt_t* rsdt);
fadt_t* find_fadt_by_xsdt(xsdt_t* xsdt);
#endif // __FADT_H__
