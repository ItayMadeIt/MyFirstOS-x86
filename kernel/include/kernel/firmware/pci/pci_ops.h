#ifndef __PCI_OPS_H__
#define __PCI_OPS_H__

#include <core/defs.h>
#define BUS_COUNT 256
#define DEVS_COUNT 32
#define FUNCS_COUNT 8

#define PCI_CMD_OFF 0x04

u8 pci_config_read_byte(u8 bus, u8 slot, u8 func, u8 offset);
u16 pci_config_read_word(u8 bus, u8 slot, u8 func, u8 offset);
u32 pci_config_read_dword(u8 bus, u8 slot, u8 func, u8 offset);

void pci_config_write_byte(u8 bus, u8 slot, u8 func, u8 offset, u8 data);
void pci_config_write_word(u8 bus, u8 slot, u8 func, u8 offset, u16 data);
void pci_config_write_dword(u8 bus, u8 slot, u8 func, u8 offset, u32 data);

#endif // __PCI_OPS_H__