#include <arch/i386/drivers/pci/ide.h>
#include <kernel/devices/storage.h>
#include <firmware/pci/pci.h>
#include <drivers/storage.h>

void init_arch_storage(storage_add_device add_func)
{
    init_ide(add_func, get_pci_device(IDE_CLASS_CODE, IDE_SUBCLASS_CODE));
}
