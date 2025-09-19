#include <drivers/pci.h>
#include <drivers/storage.h>

#define IDE_CLASS_CODE    0x01
#define IDE_SUBCLASS_CODE 0x01

void init_ide(storage_add_device add_func, pci_driver_t *driver);