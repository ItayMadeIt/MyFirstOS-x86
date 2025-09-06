#ifndef __PCI_H__
#define __PCI_H__

#include <core/defs.h>

typedef struct __attribute__((packed)) pci_common_header 
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t  revision_id;
    uint8_t  progif;
    uint8_t  subclass;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
} pci_common_header_t;

#endif // __PCI_H__