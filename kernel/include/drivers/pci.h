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
    uint8_t  subclass_code;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
} pci_common_header_t;

typedef struct __attribute__((packed)) pci_device_header {
    pci_common_header_t header;

    uint32_t base_addr[6];
    uint32_t cardbus_cis_ptr;

    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;

    uint32_t expansion_rom_base_addr;

    uint8_t  capabilities_ptr;
    uint8_t  reserved1[3];
    uint32_t reserved2;

    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint8_t  min_grant;
    uint8_t  max_latency;
} pci_device_header_t;

typedef struct __attribute__((packed)) pci_bridge_header {
    pci_common_header_t header;

    uint32_t base_addr[2];

    uint8_t  primary_bus_number;
    uint8_t  secondary_bus_number;
    uint8_t  subordinate_bus_number;
    uint8_t  secondary_latency_timer;

    uint8_t  io_base;
    uint8_t  io_limit;
    uint16_t secondary_status;

    uint16_t memory_base;
    uint16_t memory_limit;

    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;

    uint32_t prefetchable_base_upper32;
    uint32_t prefetchable_limit_upper32;

    uint16_t io_base_upper16;
    uint16_t io_limit_upper16;

    uint8_t  capabilities_ptr;
    uint8_t  reserved1[3];

    uint32_t expansion_rom_base_addr;

    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint16_t bridge_control;
} pci_bridge_header_t;

typedef struct __attribute__((packed)) pci_cardbus_bridge_header {
    pci_common_header_t header;

    uint32_t cardbus_socket_base_addr;

    uint8_t  capabilities_list_offset;
    uint8_t  reserved;
    uint16_t secondary_status;

    uint8_t  pci_bus_number;
    uint8_t  cardbus_bus_number;
    uint8_t  subordinate_bus_number;
    uint8_t  cardbus_latency_timer;

    uint32_t memory_base_addr0;
    uint32_t memory_limit0;
    uint32_t memory_base_addr1;
    uint32_t memory_limit1;

    uint32_t io_base_addr0;
    uint32_t io_limit0;
    uint32_t io_base_addr1;
    uint32_t io_limit1;

    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint16_t bridge_control;

    uint16_t subsystem_device_id;
    uint16_t subsystem_vendor_id;

    uint32_t legacy_base_addr;
} pci_cardbus_bridge_header_t;

enum header_type 
{
    PCI_HEADER_GENERAL_DEVICE = 0,
    PCI_HEADER_PCI_BRIDGE     = 1,
    PCI_HEADER_CARDBUS_BRIDGE = 2,
} ;

typedef struct pci_driver {
    uint8_t func;                // function number (0–7)
    uint8_t slot;                    // device slot (0-31)
    uint8_t bus;                     // bus number (0-255)
    union {
        pci_common_header_t         header;
        pci_device_header_t         device_header;
        pci_bridge_header_t         pci_bridge_header;
        pci_cardbus_bridge_header_t cardbus_bridge_header;
    };
    const char* device_name;

} pci_driver_t;

typedef struct pci_function {
    pci_driver_t driver;
    // next function in same device
    struct pci_function* next;
} pci_function_t;

typedef struct pci_device {
    uint8_t slot;                    // device slot (0-31)
    uint8_t bus;                     // bus number (0-255)
    pci_function_t* func_ll;
    // next device in same bus
    struct pci_device* next;
} pci_device_t;

typedef struct pci_bus {
    uint8_t bus;                     // bus number (0–255)
    pci_device_t* dev_ll;
    // next bus
    struct pci_bus* next;
} pci_bus_t;

pci_driver_t* get_pci_device(uint16_t class_code, uint16_t subclass_code);

void init_pci();

#endif // __PCI_H__