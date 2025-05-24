#include <memory/phys_alloc.h>
#include <core/paging.h>

// Max memory space 4GB:
#define MAX_MEM_SPACE 4294967296

#define BITS_IN_BIT 8

uint8_t pages_allocated[MAX_MEM_SPACE / PAGE_SIZE / BITS_IN_BIT];

extern uint8_t kernel_end_var;
uint32_t kernel_end;

static void reset_pages_allocated()
{
    for (uint32_t i = 0; i < sizeof(pages_allocated); i++)
    {
        pages_allocated[i] = 0;
    }
}

static void verify_flags(multiboot_info_t* mbd)
{
    if (! (mbd->flags & (0b1 << 6)) )
    {
        debug_print_str("Memory flag isn't enable.\n");
        halt();
    }
}

static phys_memory_list_t get_available_memory_list(multiboot_info_t* mbd)
{
    phys_memory_list_t mem_list;
    mem_list.amount = 0;
    
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t) mmap < mmap_end)
    {
        // Save into the list
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {   
            mem_list.mmmt[mem_list.amount++] = mmap;
        }
        else
        {
            debug_print(mmap->addr_low);
            debug_print(mmap->len_low);
        }

        // Get next iteration
        uint32_t next_mmap_addr = (uint32_t) mmap + (sizeof(mmap->size) + mmap->size);
        mmap = (multiboot_memory_map_t*) (next_mmap_addr);
    }

    return mem_list;
}

static void alloc_page_at(uint32_t addr)
{
    uint32_t page_index = addr >> 12;

    uint8_t bit = 1ull << (page_index & 0b111);

    uint32_t page_chunk_index = page_index >> 3; 

    pages_allocated[page_chunk_index] &= ~bit;
}

static void free_page_at(uint32_t addr)
{
    uint32_t page_index = addr >> 12;

    uint8_t bit = 1ull << (page_index & 0b111);

    uint32_t page_chunk_index = page_index >> 3; 

    pages_allocated[page_chunk_index] |= bit;
}

static void free_memory_map(multiboot_memory_map_t* map)
{
    uint32_t begin_addr = map->addr_low;
    uint32_t end_addr = map->addr_low + map->len_low;

    // Round up
    if ((begin_addr & 0xFFF) != 0)
    {
        begin_addr = (begin_addr & ~0xFFF) + 0x1000; 
    }

    // Round down
    end_addr = (end_addr & ~0xFFF) - 1;

    // Free each page
    uint32_t addr = begin_addr;
    while (addr < end_addr)
    {
        free_page_at(addr);
        
        addr += PAGE_SIZE;
    }
}

void setup_phys_allocator(multiboot_info_t* mbd)
{
    reset_pages_allocated();

    verify_flags(mbd);

    kernel_end = (uint32_t)&kernel_end_var;
    
    phys_memory_list_t mem_list = get_available_memory_list(mbd);

    for (uint32_t i = 0; i < mem_list.amount; i++)
    {
        if (mem_list.mmmt[i]->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            free_memory_map(mem_list.mmmt[i]);
        }
    }
}