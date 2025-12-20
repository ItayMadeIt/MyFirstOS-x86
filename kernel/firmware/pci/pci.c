#include "memory/heap/heap.h"
#include <firmware/pci/pci.h>
#include <kernel/firmware/pci/pci_ops.h>
#include <stdio.h>

#define INVALID_VENDOR_ID 0xFFFF
#define OFFSET_VENDOR_ID  0
#define OFFSET_HEADER_TYPE_ID 0xE

static inline u16 get_vendor_id(u8 bus, u8 dev_slot, u8 func)
{
    return pci_config_read_word(bus, dev_slot, func, OFFSET_VENDOR_ID);
}

static inline u8 get_header_type(u8 bus, u8 dev_slot, u8 func)
{
    return pci_config_read_byte(bus, dev_slot, func, OFFSET_HEADER_TYPE_ID);
}

static inline u8 is_multifunction(u8 bus, u8 dev_slot)
{
    u8 header_type = pci_config_read_byte(bus, dev_slot, 0, OFFSET_HEADER_TYPE_ID);
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

static void scan_bus(u8 bus);
static void scan_device(pci_device_t* node);
static void scan_function(pci_function_t* function);

#define PERMADE_DEVICES_NAMES 1
typedef struct device_metadata {
    u8 class_code;
    u8 subclass_code;
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
    node->driver.device_name = NULL;
    for (u32 i = 0; i < PERMADE_DEVICES_NAMES; i++)
    {
        if (devs_metadata[i].class_code == node->driver.header.class_code &&
            devs_metadata[i].subclass_code == node->driver.header.subclass_code)
        {
            node->driver.device_name = devs_metadata[i].device_name;
        }
    }
}

static void fetch_func_header(pci_function_t* node)
{
    node->driver.header.header_type = get_header_type(node->driver.bus, node->driver.slot, node->driver.func);
    u8 header_type = node->driver.header.header_type & 0x7F;

    u16 header_size = 0;
    u32* header = NULL;

    switch (header_type)
    {
        case PCI_HEADER_GENERAL_DEVICE:
        {
            header_size = sizeof(pci_device_header_t);   
            header = (u32*) &node->driver.device_header;
            break;
        }
        case PCI_HEADER_CARDBUS_BRIDGE:
        {
            header_size = sizeof(pci_cardbus_bridge_header_t);
            header = (u32*) &node->driver.cardbus_bridge_header;
            break;
        }
        case PCI_HEADER_PCI_BRIDGE:
        {
            header_size = sizeof(pci_bridge_header_t);
            header = (u32*) &node->driver.pci_bridge_header;
            break;
        }
        default:
        {
            // invalid header type
            abort();
        }
    }
    assert(header_size % sizeof(u32) == 0);

    for (u16 i = 0; i < header_size; i+=sizeof(u32))
    {
        header[i / sizeof(u32)] = pci_config_read_dword(
            node->driver.bus,
            node->driver.slot,
            node->driver.func,
            i
        );
    }
}

static pci_bus_t* make_bus_node(u8 bus)
{
    pci_bus_t* bus_node = kmalloc(sizeof(pci_bus_t));

    bus_node->bus = bus;
    bus_node->dev_ll = NULL;
    bus_node->next = NULL;

    return bus_node;
}

static pci_device_t* make_dev_node(pci_bus_t* bus_node, u8 dev_slot)
{
    pci_device_t* dev_node = kmalloc(sizeof(pci_device_t));

    dev_node->bus = bus_node->bus;
    dev_node->slot = dev_slot;
    dev_node->func_ll = NULL;
    dev_node->next = NULL;

    push_device_node(bus_node, dev_node);

    return dev_node;
}

static pci_function_t* make_func_node(pci_device_t* dev_node, u8 func_index)
{
    pci_function_t* func_node = kmalloc(sizeof(pci_function_t));

    func_node->driver.bus = dev_node->bus;
    func_node->driver.slot = dev_node->slot;
    func_node->driver.func = func_index;

    func_node->next = NULL;

    push_func_node(dev_node, func_node);

    fetch_func_header(func_node);

    fetch_dev_name(func_node);

    return func_node;
}

static int amount;

static void scan_function(pci_function_t* function)
{     
   u8 header_type = function->driver.header.header_type & 0x7F;

    if (header_type == PCI_HEADER_PCI_BRIDGE)
    {
        scan_bus(function->driver.pci_bridge_header.secondary_bus_number);
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

    for (usize_ptr func_index = 0; func_index < FUNCS_COUNT; func_index++)
    {
        u16 vendor_id = get_vendor_id(dev_node->bus, dev_node->slot, func_index);

        if (vendor_id == INVALID_VENDOR_ID)
        {
            continue;
        }
        amount++;
        
        pci_function_t* func_node = make_func_node(dev_node, func_index);
        
        scan_function(func_node);
    }
}

static void scan_bus(u8 bus)
{
    amount++;
    pci_bus_t* bus_node = make_bus_node(bus);

    for (usize_ptr dev_index = 0; dev_index < DEVS_COUNT; dev_index++)
    {
        u16 vendor_id = get_vendor_id(bus, dev_index, 0); 

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

pci_driver_t* get_pci_device(u16 class_code, u16 subclass_code)
{
    for (pci_bus_t* bus_it = pci_ll; bus_it != NULL; bus_it = bus_it->next)
    {
        for (pci_device_t* dev_it = bus_it->dev_ll; dev_it != NULL; dev_it = dev_it->next)
        {
            for (pci_function_t* func_it = dev_it->func_ll; func_it != NULL; func_it = func_it->next)
            {
                if (func_it->driver.header.class_code != class_code) continue;
                
                if (func_it->driver.header.subclass_code != subclass_code) continue;
                
                return &func_it->driver;
            }
        }
    }
    return NULL;
}

void init_pci()
{
    init_device_names();

    // scan full PCI
    pci_ll = NULL;
    scan_bus(0);

}