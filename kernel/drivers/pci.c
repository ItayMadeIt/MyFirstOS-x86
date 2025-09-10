#include "memory/heap/heap.h"
#include <drivers/pci.h>
#include <kernel/drivers/pci_ops.h>
#include <stdio.h>

#define INVALID_VENDOR_ID 0xFFFF
#define OFFSET_VENDOR_ID  0
#define OFFSET_HEADER_TYPE_ID 0xE

static inline uint16_t get_vendor_id(uint8_t bus, uint8_t dev_slot, uint8_t func)
{
    return pci_config_read_word(bus, dev_slot, func, OFFSET_VENDOR_ID);
}

static inline uint8_t get_header_type(uint8_t bus, uint8_t dev_slot, uint8_t func)
{
    return pci_config_read_byte(bus, dev_slot, func, OFFSET_HEADER_TYPE_ID);
}

static inline uint8_t is_multifunction(uint8_t bus, uint8_t dev_slot)
{
    uint8_t header_type = pci_config_read_byte(bus, dev_slot, 0, OFFSET_HEADER_TYPE_ID);
    return header_type & 0x80;
}

pci_bus_t* pci_ll;

static inline void push_bus_node(pci_bus_t* new_node)
{
    new_node->next = pci_ll;
    pci_ll = new_node;
} 
static inline void push_device_node(pci_bus_t* bus_node, pci_device_t* new_dev_node)
{
    new_dev_node->next = bus_node->dev_ll ;

    bus_node->dev_ll = new_dev_node;
} 
static inline void push_func_node(pci_device_t* dev_node, pci_function_t* new_func_node)
{
    new_func_node->next = dev_node->func_ll ;

    dev_node->func_ll = new_func_node;
} 

static void scan_bus(uint8_t bus);
static void scan_device(pci_device_t* node);
static void scan_function(pci_function_t* function);

#define PERMADE_DEVICES_NAMES 1
typedef struct device_metadata {
    uint8_t class_code;
    uint8_t subclass_code;
    const char* device_name;
} device_metadata_t;

static device_metadata_t devs_metadata[PERMADE_DEVICES_NAMES];

static void init_device_names()
{
    devs_metadata[0] = (device_metadata_t){
        0x01, 
        0x01, 
        "IDE Controller"
    };
}

static void fetch_dev_name(pci_function_t* node)
{
    node->device_name = NULL;
    for (uint32_t i = 0; i < PERMADE_DEVICES_NAMES; i++)
    {
        if (devs_metadata[i].class_code == node->header.class_code &&
            devs_metadata[i].subclass_code == node->header.subclass_code)
        {
            node->device_name = devs_metadata[i].device_name;
        }
    }
}

static void fetch_func_header(pci_function_t* node)
{
    node->header.header_type = get_header_type(node->bus, node->slot, node->function);
    uint8_t header_type = node->header.header_type & 0x7F;

    uint16_t header_size = 0;
    uint32_t* header = NULL;

    switch (header_type)
    {
        case PCI_HEADER_GENERAL_DEVICE:
        {
            header_size = sizeof(pci_device_header_t);   
            header = (uint32_t*) &node->device_header;
            break;
        }
        case PCI_HEADER_CARDBUS_BRIDGE:
        {
            header_size = sizeof(pci_cardbus_bridge_header_t);
            header = (uint32_t*) &node->cardbus_bridge_header;
            break;
        }
        case PCI_HEADER_PCI_BRIDGE:
        {
            header_size = sizeof(pci_bridge_header_t);
            header = (uint32_t*) &node->pci_bridge_header;
            break;
        }
        default:
        {
            // invalid header type
            abort();
        }
    }
    assert(header_size % sizeof(uint32_t) == 0);

    for (uint16_t i = 0; i < header_size; i+=sizeof(uint32_t))
    {
        header[i / sizeof(uint32_t)] = pci_config_read_dword(
            node->bus,
            node->slot,
            node->function,
            i
        );
    }
}

static pci_bus_t* make_bus_node(uint8_t bus)
{
    pci_bus_t* bus_node = kalloc(sizeof(pci_bus_t));

    bus_node->bus = bus;
    bus_node->dev_ll = NULL;
    bus_node->next = NULL;

    return bus_node;
}

static pci_device_t* make_dev_node(pci_bus_t* bus_node, uint8_t dev_slot)
{
    pci_device_t* dev_node = kalloc(sizeof(pci_device_t));

    dev_node->bus = bus_node->bus;
    dev_node->slot = dev_slot;
    dev_node->func_ll = NULL;
    dev_node->next = NULL;

    push_device_node(bus_node, dev_node);

    return dev_node;
}

static pci_function_t* make_func_node(pci_device_t* dev_node, uint8_t func_index)
{
    pci_function_t* func_node = kalloc(sizeof(pci_function_t));

    func_node->bus = dev_node->bus;
    func_node->slot = dev_node->slot;
    func_node->function = func_index;

    func_node->next = NULL;

    push_func_node(dev_node, func_node);

    fetch_func_header(func_node);

    fetch_dev_name(func_node);

    return func_node;
}

static int amount;

static void scan_function(pci_function_t* function)
{     
   uint8_t header_type = function->header.header_type & 0x7F;

    if (header_type == PCI_HEADER_PCI_BRIDGE)
    {
        amount++;
        scan_bus(function->pci_bridge_header.secondary_bus_number);
    }
}

static void scan_device(pci_device_t* dev_node)
{
    if (is_multifunction(dev_node->bus, dev_node->slot) == false)
    {
        amount++;
        pci_function_t* func_node = make_func_node(dev_node, 0);
        
        scan_function(func_node);

        return;
    }

    for (uintptr_t func_index = 0; func_index < FUNCS_COUNT; func_index++)
    {
        uint16_t vendor_id = get_vendor_id(dev_node->bus, dev_node->slot, func_index);

        if (vendor_id == INVALID_VENDOR_ID)
        {
            continue;
        }
        amount++;
        
        pci_function_t* func_node = make_func_node(dev_node, func_index);
        
        scan_function(func_node);
    }
}

static void scan_bus(uint8_t bus)
{
    pci_bus_t* bus_node = make_bus_node(bus);

    for (uintptr_t dev_index = 0; dev_index < DEVS_COUNT; dev_index++)
    {
        uint16_t vendor_id = get_vendor_id(bus, dev_index, 0); 

        // check first function
        if (vendor_id == INVALID_VENDOR_ID)
        {
            continue;
        }

        amount++;
        pci_device_t* dev_node = make_dev_node(bus_node, dev_index);

        scan_device(dev_node);
    }

    push_bus_node(bus_node);
}

void init_pci()
{
    init_device_names();
    
    // scan full PCI
    pci_ll = NULL;
    scan_bus(0);

    pci_bus_t* bus_it = pci_ll;
    while (bus_it)
    {
        pci_device_t* dev_it = bus_it->dev_ll;
        while (dev_it)
        {
            pci_function_t* func_it = dev_it->func_ll;
            while (func_it)
            {
                uint8_t header_type = func_it->header.header_type & 0x7F;

                if (header_type == PCI_HEADER_GENERAL_DEVICE) 
                {
                    printf("Vendor %x | Class %x Subclass %x Progif %x Name %s\n",
                        func_it->device_header.header.vendor_id,
                        func_it->device_header.header.class_code,
                        func_it->device_header.header.subclass_code,
                        func_it->device_header.header.progif, 
                        func_it->device_name ? func_it->device_name : "None");
                }
                else if (header_type == PCI_HEADER_PCI_BRIDGE) 
                {
                    printf("Vendor %x | PCI-PCI bridge | Secondary bus %d\n",
                        func_it->pci_bridge_header.header.vendor_id,
                        func_it->pci_bridge_header.secondary_bus_number);
                }
                else if (header_type == PCI_HEADER_CARDBUS_BRIDGE) 
                {
                    printf("Vendor %x | CardBus bridge\n",
                        func_it->cardbus_bridge_header.header.vendor_id);
                }


                func_it = func_it->next;
            }

            dev_it = dev_it->next;
        }
        
        bus_it = bus_it->next;
    }
    printf("\n%d\n\n", amount);
}