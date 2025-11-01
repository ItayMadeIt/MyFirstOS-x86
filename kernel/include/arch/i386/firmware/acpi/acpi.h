#include "core/num_defs.h"
#include <firmware/acpi/acpi.h>

struct acpi_timer
{
    u8 flags; // acpi_timer_flags
    union {
        struct {
            u8 unit; 
            volatile void* addr;
        } mmio;  
        u16 io_port;
    };
};