#include <drivers/pci.h>

#define IDE_CLASS_CODE    0x01
#define IDE_SUBCLASS_CODE 0x01

void init_ide(pci_driver_t* driver);