#include "boot/fadt.h"
#include "drivers/io.h"
#include <boot/acpi.h>
#include <boot/rsdp.h>
#include <boot/rsdt.h>

acpi_timer_t acpi_timer;

bool valid_checksum(acpi_sdt_header_t* table_header)
{
    unsigned char sum = 0;

    for (uint32_t i = 0; i < table_header->length; i++)
    {
        sum += ((char *) table_header)[i];
    }

    return sum == 0;
}

uint32_t get_acpi_time()
{
    if (acpi_timer.flags & ACPI_TIMER_MMIO)
    {
        // No support yet
        return 0;
    }
    else if (acpi_timer.flags & ACPI_TIMER_IO)
    {
        return inl(acpi_timer.io_port);
    }
    
    return 0;
}

void setup_acpi()
{
    rsdp_t* rdsp = gather_rdsp();
    
    fadt_t* fadt = NULL;

    if (rdsp->revision >= 2 && ((xsdp_t*)rdsp)->xsdt_address != 0) 
    {
        xsdt_t* xsdt = (xsdt_t*)(uintptr_t)((xsdp_t*)rdsp)->xsdt_address;
        fadt = find_fadt_by_xsdt(xsdt);
    } 
    else 
    {
        rsdt_t* rsdt = (rsdt_t*)(uintptr_t)rdsp->rsdt_address;
        fadt = find_fadt_by_rsdt(rsdt);
    }

    assert (fadt);
    assert (fadt->PM_timer_length == 4);

    acpi_timer.flags = ACPI_TIMER_ACTIVE;

    if (rdsp->revision >= 2 && fadt->X_PM_timer_block.address != (uint32_t)NULL)
    {
        acpi_timer.flags |= ACPI_TIMER_MMIO;

        acpi_timer.mem_region = fadt->X_PM_timer_block;
    }
    else
    {
        acpi_timer.flags |= ACPI_TIMER_IO;

        acpi_timer.io_port = fadt->PM_timer_block;
    }

    // bit 8 == 32 bits
    if (fadt->flags & 0x100) 
    {
        acpi_timer.flags &= ~ACPI_TIMER_24;
    }
    else
    {
        acpi_timer.flags |= ACPI_TIMER_24;
    }
}
