#include <firmware/acpi/fadt.h>
#include <firmware/acpi/rsdp.h>
#include <firmware/acpi/rsdt.h>

bool valid_checksum(acpi_sdt_header_t* table_header)
{
    unsigned char sum = 0;

    for (uint32_t i = 0; i < table_header->length; i++)
    {
        sum += ((char *) table_header)[i];
    }

    return sum == 0;
}
