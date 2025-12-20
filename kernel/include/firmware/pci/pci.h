#ifndef __PCI_H__
#define __PCI_H__

#include <core/defs.h>

typedef struct __attribute__((packed)) pci_common_header 
{
    u16 vendor_id;
    u16 device_id;
    u16 command;
    u16 status;
    u8  revision_id;
    u8  progif;
    u8  subclass_code;
    u8  class_code;
    u8  cache_line_size;
    u8  latency_timer;
    u8  header_type;
    u8  bist;
} pci_common_header_t;

typedef struct __attribute__((packed)) pci_device_header {
    pci_common_header_t header;

    u32 base_addr[6];
    u32 cardbus_cis_ptr;

    u16 subsystem_vendor_id;
    u16 subsystem_id;

    u32 expansion_rom_base_addr;

    u8  capabilities_ptr;
    u8  reserved1[3];
    u32 reserved2;

    u8  interrupt_line;
    u8  interrupt_pin;
    u8  min_grant;
    u8  max_latency;
} pci_device_header_t;

typedef struct __attribute__((packed)) pci_bridge_header {
    pci_common_header_t header;

    u32 base_addr[2];

    u8  primary_bus_number;
    u8  secondary_bus_number;
    u8  subordinate_bus_number;
    u8  secondary_latency_timer;

    u8  io_base;
    u8  io_limit;
    u16 secondary_status;

    u16 memory_base;
    u16 memory_limit;

    u16 prefetchable_memory_base;
    u16 prefetchable_memory_limit;

    u32 prefetchable_base_upper32;
    u32 prefetchable_limit_upper32;

    u16 io_base_upper16;
    u16 io_limit_upper16;

    u8  capabilities_ptr;
    u8  reserved1[3];

    u32 expansion_rom_base_addr;

    u8  interrupt_line;
    u8  interrupt_pin;
    u16 bridge_control;
} pci_bridge_header_t;

typedef struct __attribute__((packed)) pci_cardbus_bridge_header {
    pci_common_header_t header;

    u32 cardbus_socket_base_addr;

    u8  capabilities_list_offset;
    u8  reserved;
    u16 secondary_status;

    u8  pci_bus_number;
    u8  cardbus_bus_number;
    u8  subordinate_bus_number;
    u8  cardbus_latency_timer;

    u32 memory_base_addr0;
    u32 memory_limit0;
    u32 memory_base_addr1;
    u32 memory_limit1;

    u32 io_base_addr0;
    u32 io_limit0;
    u32 io_base_addr1;
    u32 io_limit1;

    u8  interrupt_line;
    u8  interrupt_pin;
    u16 bridge_control;

    u16 subsystem_device_id;
    u16 subsystem_vendor_id;

    u32 legacy_base_addr;
} pci_cardbus_bridge_header_t;

enum header_type 
{
    PCI_HEADER_GENERAL_DEVICE = 0,
    PCI_HEADER_PCI_BRIDGE     = 1,
    PCI_HEADER_CARDBUS_BRIDGE = 2,
} ;

typedef struct pci_driver {
    u8 func;                // function number (0–7)
    u8 slot;                    // device slot (0-31)
    u8 bus;                     // bus number (0-255)
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
    u8 slot;                    // device slot (0-31)
    u8 bus;                     // bus number (0-255)
    pci_function_t* func_ll;
    // next device in same bus
    struct pci_device* next;
} pci_device_t;

typedef struct pci_bus {
    u8 bus;                     // bus number (0–255)
    pci_device_t* dev_ll;
    // next bus
    struct pci_bus* next;
} pci_bus_t;

pci_driver_t* get_pci_device(u16 class_code, u16 subclass_code);

void init_pci();

#endif // __PCI_H__