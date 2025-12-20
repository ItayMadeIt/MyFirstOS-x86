#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/core/init.h>
#include <kernel/boot/boot_data.h>
#include <kernel/core/cpu.h>
#include <kernel/core/paging.h>
#include <memory/core/pfn_desc.h>
#include <memory/core/linker_vars.h>
#include <stdio.h>

static u8 pages_allocated[PAGES_ARR_SIZE];
static usize_ptr last_free_group_index = 0;

static void reset_pages_allocated()
{
    usize_ptr pages_amount = max_memory / PAGE_SIZE;
    for (usize_ptr i = 0; i < pages_amount; i++)
    {
        pages_allocated[i] = 0;
    }

    last_free_group_index = 0;
}

static void alloc_page_at(usize_ptr addr)
{
    usize_ptr page_index = addr / PAGE_SIZE;

    u8 bit = 1ull << (page_index % BIT_TO_BYTE);

    usize_ptr page_chunk_index = page_index / BIT_TO_BYTE; 

    pages_allocated[page_chunk_index] &= ~bit;
}

static void free_page_at(void* addr_ptr)
{
    usize_ptr addr = (usize_ptr)addr_ptr;
    usize_ptr page_index = addr / PAGE_SIZE;

    u8 bit = 1ull << (page_index % BIT_TO_BYTE);

    usize_ptr page_chunk_index = page_index / BIT_TO_BYTE; 

    pages_allocated[page_chunk_index] |= bit;
}

static void alloc_page_region(void* from_addr, void* to_addr)
{
    usize_ptr addr = (usize_ptr)from_addr;

    while (addr < (usize_ptr)to_addr)
    {
        alloc_page_at(addr);
        
        addr += PAGE_SIZE;
    }
}

// static void free_page_region(usize_ptr from_addr, usize_ptr to_addr)
// {
//     usize_ptr addr = from_addr;
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
    kernel_size = round_page_up(kernel_end_pa) - round_page_down(kernel_begin_pa);
    
    // boot_foreach_free_page_region(boot_data, alloc_page_region); 
}

void* bitmap_alloc_page()
{
    phys_alloc_t alloc = bitmap_alloc_pages(1);

    return alloc.count ? alloc.addr : NULL;
}

phys_alloc_t bitmap_alloc_pages(usize_ptr count)
{
    phys_alloc_t result = { .addr = NULL, .count = 0 };

    if (count == 0) 
    {
        return result;
    }


    // Find free page group
    usize_ptr page_group_index;
    for (page_group_index = last_free_group_index; page_group_index < PAGES_ARR_SIZE; page_group_index++)
    {
        if (pages_allocated[page_group_index])
        {
            break;
        }
    }
    
    // Number of pages available in bitmap
    usize_ptr total_pages = PAGES_ARR_SIZE * BIT_TO_BYTE;
    usize_ptr run_start = (usize_ptr)-1;
    usize_ptr run_length = 0;

    for (usize_ptr i = page_group_index * BIT_TO_BYTE; i < total_pages; i++) 
    {
        usize_ptr group = i / BIT_TO_BYTE;
        usize_ptr bit   = i % BIT_TO_BYTE;
        bool is_free = pages_allocated[group] & (1 << bit);

        // hit allocated page before count 
        if (!is_free && run_start != (usize_ptr)-1) 
        {
            break;
        }

        if (is_free)
        {
            // start new run
            if (run_start == (usize_ptr)-1) 
            {
                run_start = i;
                
            }
            run_length++;

            // enough pages?
            if (run_length == count) 
            {
                break;
            }
        }
    }

    // no free page found at all
    if (run_start == (usize_ptr)-1)
    {
        return result;
    }

    usize_ptr base_addr = run_start * PAGE_SIZE;
    usize_ptr allocated = run_length;
    for (usize_ptr j = 0; j < allocated; j++) 
    {
        alloc_page_at((run_start + j) * PAGE_SIZE);
    }

    result.addr  = (void*)base_addr;
    result.count = allocated;

    return result;
}

void free_phys_page_bitmap(void* page_addr)
{
    free_page_at(page_addr);

    // (8 pages per byte in bitmap)
    last_free_group_index = (usize_ptr)page_addr / PAGE_SIZE / BIT_TO_BYTE;
}

void free_phys_pages_bitmap(phys_alloc_t free_params)
{
    for (usize_ptr i = 0; i < free_params.count; i++)
    {
        usize_ptr addr = (usize_ptr)free_params.addr + i * PAGE_SIZE;

        free_phys_page_bitmap((void*)addr);
    }
}

void init_bitmap_phys_allocator(boot_data_t* boot_data)
{
    reset_pages_allocated();

    boot_foreach_free_page(boot_data, free_page_at);
    boot_foreach_reserved_region(boot_data, alloc_page_region);

    reserve_kernel_memory();
}