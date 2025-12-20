#include <kernel/firmware/pci/pci_ops.h>
#include <arch/i386/drivers/io/io.h>

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline void pci_write_address(u8 bus, u8 slot, u8 func, u8 offset) 
{
    u32 address = (1u << 31) // enable bit
                     | ((u32)bus  << 16)
                     | ((u32)slot << 11)
                     | ((u32)func << 8)
                     | (offset & 0xFC); // align to 4 bytes

    outl(PCI_CONFIG_ADDR, address);
}


u8 pci_config_read_byte(u8 bus, u8 slot, u8 func, u8 offset) 
{
    pci_write_address(bus, slot, func, offset);

    u32 val = inl(PCI_CONFIG_DATA);

    return (u8)((val >> ((offset & 0b11) * 8)) & 0xFF);
}

u16 pci_config_read_word(u8 bus, u8 slot, u8 func, u8 offset)
{
    pci_write_address(bus, slot, func, offset);

    u32 val = inl(PCI_CONFIG_DATA);

    return (u16)((val >> ((offset & 0b10) * 8)) & 0xFFFFu);
}

u32 pci_config_read_dword(u8 bus, u8 slot, u8 func, u8 offset)
{
    pci_write_address(bus, slot, func, offset);

    u32 val = inl(PCI_CONFIG_DATA);

    return val;
}

void pci_config_write_byte(u8 bus, u8 slot, u8 func, u8 offset, u8 data) 
{
    pci_write_address(bus, slot, func, offset);
    
    u32 val = inl(PCI_CONFIG_DATA);
    u32 shift = (offset & 0b11) * 8;
    
    val = (val & ~(0xFFu << shift)); 

    val |= ((u32)data << shift);

    outl(PCI_CONFIG_DATA, val);
}

void pci_config_write_word(u8 bus, u8 slot, u8 func, u8 offset, u16 data)
{
    pci_write_address(bus, slot, func, offset);

    u32 val = inl(PCI_CONFIG_DATA);
    u32 shift = (offset & 0b10) * 8;
    
    val = (val & ~(0xFFFFu << shift)); 

    val |= ((u32)data << shift);

    outl(PCI_CONFIG_DATA, val);
}

void pci_config_write_dword(u8 bus, u8 slot, u8 func, u8 offset, u32 data)
{
    pci_write_address(bus, slot, func, offset);

    outl(PCI_CONFIG_DATA, data);
}
