#ifndef __PCI_OPS_H__
#define __PCI_OPS_H__

#include <core/defs.h>

uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_config_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);

#endif // __PCI_OPS_H__