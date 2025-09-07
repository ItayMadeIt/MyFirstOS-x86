#include <core/defs.h>
#include <string.h>
#include <firmware/rsdt.h>
#include <firmware/fadt.h>

#define FADT_SIGNATURE "FACP"
#define FADT_SIGNATURE_LEN 4

fadt_t* find_fadt_by_rsdt(rsdt_t* rsdt)
{
    int entries = (rsdt->header.length - sizeof(rsdt->header.length)) / 4;

    for (int i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *h = (acpi_sdt_header_t*)rsdt->pointer_sdts[i];

        if (!strncmp(h->signature, FADT_SIGNATURE, FADT_SIGNATURE_LEN))
            return (void *) h;
    }

    return NULL;
} 
fadt_t* find_fadt_by_xsdt(xsdt_t* rsdt)
{
    int entries = (rsdt->header.length - sizeof(rsdt->header.length)) / 8;

    for (int i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *h = (acpi_sdt_header_t*)rsdt->pointer_sdts[i];

        if (!strncmp(h->signature, FADT_SIGNATURE, FADT_SIGNATURE_LEN))
            return (void *) h;
    }

    return NULL;
} 