#include <core/defs.h>
#include <early/defs.h>
#include <arch/i386/core/paging.h>

#define KERNEL_TABLES 64
#define BOOT_TABLES   2

#define PAGES_UP(x)   ( ((x) + (PAGE_SIZE-1)) / PAGE_SIZE)


/* ---- linker fenceposts ---- */
extern char __kernel_phys_base_sym[];   // End of boot (phys), Start of kernel (phys) 
extern char __kernel_virt_base_sym[];   // high VMA base for kernel proper 
extern char __kernel_start[];           // high VMA start of kernel (.text) 
extern char __image_end[];              // high VMA end of image 
extern char __va_pa_off[];              // constant offset: virt = phys + off

EARLY_BSS_SECTION
page_directory_t page_directory;
EARLY_BSS_SECTION
static page_table_t kernel_tables[KERNEL_TABLES];
EARLY_BSS_SECTION
static page_table_t boot_tables[BOOT_TABLES];

EARLY_TEXT_SECTION
void zero_page_directory(page_directory_t* page_dir)
{
    for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
    {
        page_dir->entries[i] = 0;
    }
}

EARLY_TEXT_SECTION
void zero_page_table(page_table_t* page_table, uint32_t amount)
{
    do
    {
        for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
        {
            page_table->entries[i] = 0;
        }

        --amount;
        ++page_table;

    } while(amount);
}

EARLY_TEXT_SECTION
static inline void set_cr3(page_directory_t* page_directory)
{
    asm volatile (
        "mov %0, %%cr3"
        :
        : "r" (page_directory)
    );
} 

EARLY_TEXT_SECTION
static void map_span(page_directory_t* pd,
    page_table_t** free_pt,
    uint32_t vaddr,
    uint32_t paddr,
    uint32_t pages,
    uint16_t flags)
{
    while (pages)
    {
        uint32_t dir_index = vaddr >> 22;
        uint32_t table_index = (vaddr >> 12) & 0x3FF;

        // choose the PT this PDE points to (or install a new one)
        page_table_t* pt;

        uint32_t pde_val = (uint32_t)pd->entries[dir_index];
        
        if ((pde_val & PAGE_ENTRY_FLAG_PRESENT) == 0)
        {
            // take next free table
            pt = *free_pt;
            for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
            {
                pt->entries[i] = 0;
            }

            pd->entries[dir_index] = (void*)(((uint32_t)pt & ~0xFFF) | flags);

            // advance free list
            (*free_pt)++;
        } 
        else 
        {
            // reuse existing table
            pt = (page_table_t*)(pde_val & ~0xFFF);  
        }

        // fill PTEs within this table
        while (table_index < ENTRIES_AMOUNT && pages) 
        {
            pt->entries[table_index] = (void*)(paddr | flags);

            paddr += PAGE_SIZE;
            vaddr += PAGE_SIZE;

            --pages;
            ++table_index;
        }
    }
}


EARLY_TEXT_SECTION
static inline void enable_paging()
{
    uint32_t cr0;
    asm volatile(
        "mov %%cr0, %0\n\t"
        "or  $0x80000000, %0\n\t"
        "mov %0, %%cr0\n\t"
        /* serialize fetch after PG flip just to be extra safe */
        "jmp 1f\n\t"
        "1:\n\t"
        : "=&r"(cr0)
        :
        : "memory", "cc"
    );
}

EARLY_TEXT_SECTION
void init_paging()
{
    zero_page_directory(&page_directory);
    zero_page_table(kernel_tables, KERNEL_TABLES);
    zero_page_table(boot_tables, BOOT_TABLES);

    uint32_t pa_boot_base  = STOR_1MiB;
    uint32_t off    = (uint32_t)__va_pa_off;                      // virt = phys + off
    uint32_t va_kernel_base = (uint32_t)__kernel_start;           // high VMA start
    uint32_t pa_kernel_base = (uint32_t)__kernel_phys_base_sym;      // LMA for high start
    
    uint32_t kernel_pages = PAGES_UP((uint32_t)(__image_end - __kernel_start));
    uint32_t boot_pages   = PAGES_UP(pa_kernel_base - pa_boot_base);

    // Identity-map first 4 MiB using boot_tables
    page_table_t* free_boot_pt = boot_tables;
    map_span(
        &page_directory, 
        &free_boot_pt,
        0x00000000, 0x00000000,
        ENTRIES_AMOUNT, 
        PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE
    );


    // High alias for boot/entry span 
    page_table_t* free_kern_pt = kernel_tables;
    map_span(
        &page_directory, 
        &free_kern_pt,
        pa_boot_base + off,
        pa_boot_base,
        boot_pages, 
        PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE
    );


    // High map for kernel code
    map_span(
        &page_directory, 
        &free_kern_pt,
        va_kernel_base, 
        pa_kernel_base,
        kernel_pages, 
        PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE
    );

    // Recursive paging
    page_directory.entries[ENTRIES_AMOUNT-1] =
        (void*)(((uint32_t)&page_directory) | PAGE_ENTRY_WRITE_KERNEL_FLAGS);

    // Load and continue
    set_cr3(&page_directory);
    enable_paging();
}
