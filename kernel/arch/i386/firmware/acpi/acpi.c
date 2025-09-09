#include <arch/i386/drivers/io/io.h>
#include <arch/i386/firmware/acpi/acpi.h>
#include <memory/core/pfn_desc.h>
#include <kernel/memory/virt_alloc.h>
#include <firmware/acpi/fadt.h>
#include <firmware/acpi/rsdp.h>
#include <firmware/acpi/rsdt.h>

static acpi_timer_t acpi_timer;

uint64_t get_acpi_time()
{
    if (acpi_timer.flags & ACPI_TIMER_MMIO)
    {
        switch (acpi_timer.mmio.unit)
        {
        case 1:
            return *(uint8_t*)acpi_timer.mmio.addr;
        case 2:
            return *(uint16_t*)acpi_timer.mmio.addr;
        case 4:
            return *(uint32_t*)acpi_timer.mmio.addr;
        case 8:
            return *(uint64_t*)acpi_timer.mmio.addr;
        
        default:
            break;
        } 
        
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

    if (rdsp->revision >= 2 && fadt->X_PM_timer_block.address != (uint32_t)NULL &&
        fadt->X_PM_timer_block.address_space == 0)
    {
        acpi_timer.flags |= ACPI_TIMER_MMIO;

        generic_address_struct_t* gas = &fadt->X_PM_timer_block;
        uint32_t unit = gas_unit_size(
            gas->access_size, 
            gas->bit_width ? gas->bit_width : 32
        );
        uint32_t pa = fadt->X_PM_timer_block.address & ~(unit - 1);
        const uint32_t off = (uint32_t)(gas->address & (unit - 1));
        
        void* va = identity_map_pages(
            (void*)pa, 
            1, 
            PAGETYPE_MMIO, 
            PAGEFLAG_KERNEL | PAGEFLAG_DRIVER  | PAGEFLAG_READONLY
        );

        acpi_timer.mmio.unit = unit;
        acpi_timer.mmio.addr = (void*)((uint32_t)va + off);
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
