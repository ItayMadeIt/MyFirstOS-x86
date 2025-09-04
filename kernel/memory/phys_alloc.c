#include <core/defs.h>
#include <core/paging.h>
#include <core/debug.h>
#include <memory/phys_alloc.h>

// Max memory space 4GB:
#define MAX_MEM_SPACE 4294967296


#define PAGES_ARR_SIZE MAX_MEM_SPACE / PAGE_SIZE / BIT_TO_BYTE

static uint8_t pages_allocated[PAGES_ARR_SIZE];
static uint32_t last_free_group_index = 0;

uint64_t max_memory;

uint32_t kernel_begin_pa;
uint32_t kernel_end_pa;

static void reset_pages_allocated()
{
    uint32_t pages_amount = max_memory / PAGE_SIZE;
    for (uint32_t i = 0; i < pages_amount; i++)
    {
        pages_allocated[i] = 0;
    }

    last_free_group_index = 0;
}

static void verify_flags(const multiboot_info_t* mbd)
{
    if (! (mbd->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        debug_print_str("Memory flag isn't enable.\n");
        halt();
    }
}

static phys_memory_list_t get_available_memory_list(const multiboot_info_t* mbd)
{
    phys_memory_list_t mem_list;
    mem_list.amount = 0;
    
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t) mmap < mmap_end)
    {
        if (mem_list.amount == MAX_MEMORY_ENTRIES)
        {
            halt();
        }
        // Save into the list
        mem_list.mmmt[mem_list.amount++] = mmap;

        max_memory = max(
            max_memory,
            round_page_up( mmap->addr_low + mmap->len_low) 
        );

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

static void alloc_page_region(uint32_t from_addr, uint32_t to_addr)
{
    uint32_t addr = from_addr;

    while (addr < to_addr)
    {
        alloc_page_at(addr);
        
        addr += PAGE_SIZE;
    }
}

// static void free_page_region(uint32_t from_addr, uint32_t to_addr)
// {
//     uint32_t addr = from_addr;
//     while (addr < to_addr)
//     {
//         free_page_at(addr);
        
//         addr += PAGE_SIZE;
//     }
// }

static void free_memory_map(const multiboot_memory_map_t* map)
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

static void free_phys_mem_list(const phys_memory_list_t* mem_list)
{
    for (uint32_t i = 0; i < mem_list->amount; i++)
    {
        if (mem_list->mmmt[i]->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            free_memory_map(mem_list->mmmt[i]);
        }
    }
}

static void reserve_kernel_memory(const phys_memory_list_t* mem_list)
{
    // Round addresses
    alloc_page_region(kernel_begin_pa & ~0xFFF, (kernel_end_pa | 0xFFF) + 1);

    for (uint16_t i = 0; i < mem_list->amount; i++)
    {
        alloc_page_region(mem_list->mmmt[i]->addr_low & ~0xFFF, (mem_list->mmmt[i]->addr_high | 0xFFF) + 1);
    }
}

void* alloc_phys_page_bitmap(uint32_t page_type, uint32_t page_flags)
{
    (void)page_type; (void)page_flags;

    uint32_t page_group_index = last_free_group_index;
    
    // Find free page group
    while (page_group_index < PAGES_ARR_SIZE)
    {
        if (pages_allocated[page_group_index])
        {
            break;
        }

        ++page_group_index;
    }
    
    if (page_group_index >= PAGES_ARR_SIZE)
    {
        debug_print_str("Failed to allocate page");
        halt();
    }

    // Get page index (max 8 bits)
    uint32_t page_index = 0;
    while ((pages_allocated[page_group_index] & (1 << page_index)) == 0)
    {
        ++page_index;
    }
    page_index |= page_group_index << 3;
    // Cache
    last_free_group_index = page_group_index;

    // Allocate page
    uint32_t page_addr = page_index * PAGE_SIZE;
    alloc_page_at(page_addr);

    return (void*)page_addr;
}

void free_phys_page_bitmap(uint32_t page_addr)
{
    free_page_at(page_addr);

    // >> 12 (PAGE_SIZE) + >> 3 (8 pages per byte in bitmap)
    last_free_group_index = page_addr >> 15;
}

void setup_phys_allocator(multiboot_info_t* mbd)
{
    kernel_begin_pa = (uint32_t)&linker_kernel_begin;
    kernel_end_pa = (uint32_t)&linker_kernel_end;

    verify_flags(mbd);

    reset_pages_allocated();

    phys_memory_list_t mem_list = get_available_memory_list(mbd);
    free_phys_mem_list(&mem_list);

    alloc_page_region(0, STOR_1MiB);

    reserve_kernel_memory(&mem_list);
}