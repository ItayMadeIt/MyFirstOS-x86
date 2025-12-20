#include "memory/core/pfn_desc.h"
#include "memory/virt/virt_alloc.h"
#include "memory/virt/virt_map.h"
#include <arch/i386/drivers/io/io.h>
#include <arch/i386/firmware/acpi/acpi.h>
#include <memory/core/memory_manager.h>
#include <firmware/acpi/fadt.h>
#include <firmware/acpi/rsdp.h>
#include <firmware/acpi/rsdt.h>
#include <kernel/memory/paging.h>

static acpi_timer_t acpi_timer;

u64 get_acpi_time()
{
    if (acpi_timer.flags & ACPI_TIMER_MMIO)
    {
        switch (acpi_timer.mmio.unit)
        {
        case 1:
            return *(u8*)acpi_timer.mmio.addr;
        case 2:
            return *(u16*)acpi_timer.mmio.addr;
        case 4:
            return *(u32*)acpi_timer.mmio.addr;
        case 8:
            return *(u64*)acpi_timer.mmio.addr;
        
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

void init_acpi()
{
    rsdp_t* rdsp = gather_rdsp();
    
    fadt_t* fadt = NULL;

    if (rdsp->revision >= 2 && ((xsdp_t*)rdsp)->xsdt_address != 0) 
    {
        xsdt_t* xsdt = (xsdt_t*)(usize_ptr)((xsdp_t*)rdsp)->xsdt_address;
        fadt = find_fadt_by_xsdt(xsdt);
    } 
    else 
    {
        rsdt_t* rsdt = (rsdt_t*)(usize_ptr)rdsp->rsdt_address;
        fadt = find_fadt_by_rsdt(rsdt);
    }

    assert (fadt);
    assert (fadt->PM_timer_length == 4);

    acpi_timer.flags = ACPI_TIMER_ACTIVE;

    if (rdsp->revision >= 2 && fadt->X_PM_timer_block.address != (u32)NULL &&
        fadt->X_PM_timer_block.address_space == 0)
    {
        acpi_timer.flags |= ACPI_TIMER_MMIO;

        generic_address_struct_t* gas = &fadt->X_PM_timer_block;
        u32 unit = gas_unit_size(
            gas->access_size, 
            gas->bit_width ? gas->bit_width : 32
        );
        u32 pa = fadt->X_PM_timer_block.address & ~(unit - 1);
        const u32 off = (u32)(gas->address & (unit - 1));
        
        // no need to force alloc it somehow, it's not part of free mem space
        pfn_mark_range(
            (void*)pa, 1, 
            PAGETYPE_ACPI, 
            0
        );
        vmap_identity(
            (void*)pa, 
            1, 
            VREGION_MMIO
        );

        acpi_timer.mmio.unit = unit;
        acpi_timer.mmio.addr = (void*)(pa + off);
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
