#include <arch/i386/drivers/io.h>
#include <memory/page_frame.h>
#include <memory/virt_alloc.h>
#include <firmware/fadt.h>
#include <firmware/acpi.h>
#include <firmware/rsdp.h>
#include <firmware/rsdt.h>

acpi_timer_t acpi_timer;

uint32_t gas_unit_size(uint8_t access_size, uint8_t bit_width) 
{
    if (access_size >= 1 && access_size <= 4) 
    {
        static const uint32_t sz[5] = {0,1,2,4,8};
        return sz[access_size];
    }

    return (bit_width <= 8) ? 1 : 
            (bit_width <= 16) ? 2 : 
            (bit_width <= 32) ? 4 : 8;
}

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
