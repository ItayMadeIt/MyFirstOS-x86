#include <core/debug.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <kernel/boot/boot_data.h>
#include <kernel/core/cpu.h>
#include <kernel/core/paging.h>
#include <memory/core/pfn_desc.h>
#include <memory/core/linker_vars.h>

static uint8_t pages_allocated[PAGES_ARR_SIZE];
static uintptr_t last_free_group_index = 0;

uintptr_t max_memory;

uintptr_t kernel_begin_pa;
uintptr_t kernel_end_pa;

static void reset_pages_allocated()
{
    uintptr_t pages_amount = max_memory / PAGE_SIZE;
    for (uintptr_t i = 0; i < pages_amount; i++)
    {
        pages_allocated[i] = 0;
    }

    last_free_group_index = 0;
}

static void verify_flags(const boot_data_t* boot_data)
{
    if (!boot_has_memory(boot_data))
    {
        debug_print_str("Memory flag isn't enable.\n");
        cpu_halt();
    }
}

static void alloc_page_at(uintptr_t addr)
{
    uintptr_t page_index = addr / PAGE_SIZE;

    uint8_t bit = 1ull << (page_index & 0b111);

    uintptr_t page_chunk_index = page_index >> 3; 

    pages_allocated[page_chunk_index] &= ~bit;
}

static void free_page_at(void* addr_ptr)
{
    uintptr_t addr = (uintptr_t)addr_ptr;
    uintptr_t page_index = addr >> 12;

    uint8_t bit = 1ull << (page_index & 0b111);

    uintptr_t page_chunk_index = page_index >> 3; 

    pages_allocated[page_chunk_index] |= bit;
}

static void alloc_page_region(void* from_addr, void* to_addr)
{
    uintptr_t addr = (uintptr_t)from_addr;

    while (addr < (uintptr_t)to_addr)
    {
        alloc_page_at(addr);
        
        addr += PAGE_SIZE;
    }
}

// static void free_page_region(uintptr_t from_addr, uintptr_t to_addr)
// {
//     uintptr_t addr = from_addr;
//     while (addr < to_addr)
//     {
//         free_page_at(addr);
        
//         addr += PAGE_SIZE;
//     }
// }

static void reserve_kernel_memory()
{
    // Round addresses
    alloc_page_region(
        (void*)round_page_down(kernel_begin_pa), 
        (void*)round_page_up(kernel_end_pa)
    );
    
    // boot_foreach_free_page_region(boot_data, alloc_page_region); 
}

void* alloc_phys_page_bitmap(enum phys_page_type page_type, uint16_t page_flags)
{
    (void)page_type; (void)page_flags;

    uintptr_t page_group_index = last_free_group_index;
    
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
        cpu_halt();
    }

    // Get page index (max 8 bits)
    uintptr_t page_index = 0;
    while ((pages_allocated[page_group_index] & (1 << page_index)) == 0)
    {
        ++page_index;
    }
    page_index |= page_group_index << 3;
    // Cache
    last_free_group_index = page_group_index;

    // Allocate page
    uintptr_t page_addr = page_index * PAGE_SIZE;
    alloc_page_at(page_addr);

    return (void*)page_addr;
}

void free_phys_page_bitmap(void* page_addr)
{
    free_page_at(page_addr);

    // (8 pages per byte in bitmap)
    last_free_group_index = (uintptr_t)page_addr / PAGE_SIZE / BIT_TO_BYTE;
}

void init_bitmap_phys_allocator(boot_data_t* boot_data)
{
    kernel_begin_pa = (uintptr_t)&linker_kernel_begin;
    kernel_end_pa = (uintptr_t)&linker_kernel_end;

    verify_flags(boot_data);

    reset_pages_allocated();

    max_memory = get_max_memory(boot_data);

    boot_foreach_free_page(boot_data, free_page_at);

    boot_foreach_reserved_region(boot_data, alloc_page_region);

    reserve_kernel_memory();
}