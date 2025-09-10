#include <kernel/drivers/pci_ops.h>
#include <arch/i386/drivers/io/io.h>

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline void pci_write_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) 
{
    uint32_t address = (1u << 31) // enable bit
                     | ((uint32_t)bus  << 16)
                     | ((uint32_t)slot << 11)
                     | ((uint32_t)func << 8)
                     | (offset & 0xFC); // align to 4 bytes

    outl(PCI_CONFIG_ADDR, address);
}


uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) 
{
    pci_write_address(bus, slot, func, offset);

    uint32_t val = inl(PCI_CONFIG_DATA);

    return (uint8_t)((val >> ((offset & 0b11) * 8)) & 0xFF);
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    pci_write_address(bus, slot, func, offset);

    uint32_t val = inl(PCI_CONFIG_DATA);

    return (uint16_t)((val >> ((offset & 0b10) * 8)) & 0xFFFFu);
}

uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    pci_write_address(bus, slot, func, offset);

    uint32_t val = inl(PCI_CONFIG_DATA);

    return val;
}

void pci_config_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data) 
{
    pci_write_address(bus, slot, func, offset);
    
    uint32_t val = inl(PCI_CONFIG_DATA);
    uint32_t shift = (offset & 0b11) * 8;
    
    val = (val & ~(0xFFu << shift)); 

    val |= ((uint32_t)data << shift);

    outl(PCI_CONFIG_DATA, val);
}

void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data)
{
    pci_write_address(bus, slot, func, offset);

    uint32_t val = inl(PCI_CONFIG_DATA);
    uint32_t shift = (offset & 0b10) * 8;
    
    val = (val & ~(0xFFFFu << shift)); 

    val |= ((uint32_t)data << shift);

    outl(PCI_CONFIG_DATA, val);
}

void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data)
{
    pci_write_address(bus, slot, func, offset);

    outl(PCI_CONFIG_DATA, data);
}
