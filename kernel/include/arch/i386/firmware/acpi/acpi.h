#include <stdint.h>
#include <firmware/acpi/acpi.h>

struct acpi_timer
{
    uint8_t flags; // acpi_timer_flags
    union {
        struct {
            uint8_t unit; 
            volatile void* addr;
        } mmio;  
        uint16_t io_port;
    };
};