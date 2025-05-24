#include <multiboot/multiboot.h>

#define MAX_MEMORY_ENTRIES 256

typedef struct phys_memory_list
{
    multiboot_memory_map_t* mmmt[MAX_MEMORY_ENTRIES];
    uint16_t amount;
} phys_memory_list_t;


uint32_t alloc_phys_page();

void free_phys_page(uint32_t page_addr);

void setup_phys_allocator(multiboot_info_t* mbd);