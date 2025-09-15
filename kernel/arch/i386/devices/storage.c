#include <arch/i386/drivers/pci/ide.h>
#include <kernel/devices/storage.h>
#include <drivers/pci.h>

void init_stor()
{
    init_ide(get_pci_device(IDE_CLASS_CODE, IDE_SUBCLASS_CODE));
}