#include "core/debug.h"
#include "memory/phys_alloc.h"
#include <memory/page_frame.h>
#include <memory/paging_utils.h>
#include <core/paging.h>
#include <core/defs.h>
#include <memory/virt_alloc.h>
#include <kernel/core/cpu.h>

#include <stdint.h>
#include <string.h>

#define INIT_SIZE STOR_16Kib

uint32_t multiboot_data_end_pa(const multiboot_info_t* mbd)
{
    uint32_t end = (uint32_t)mbd + sizeof(*mbd);

    if (mbd->flags & MULTIBOOT_INFO_CMDLINE)
    {
        end = max(end, (uint32_t)mbd->cmdline + (uint32_t)strlen((char*)mbd->cmdline) + 1);
    }

    if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
    {
        end = max(end, (uint32_t)mbd->boot_loader_name + (uint32_t)strlen((char*)mbd->boot_loader_name) + 1);
    }

    if (mbd->flags & MULTIBOOT_INFO_MODS) 
    {
        struct multiboot_mod_list* mods = (void*)(uint32_t)mbd->mods_addr;
        end = max(end, (uint32_t)mbd->mods_addr + mbd->mods_count * (uint32_t)sizeof(*mods));

        for (uint32_t i = 0; i < mbd->mods_count; i++) 
        {
            end = max(end, mods[i].mod_end);
            if (mods[i].cmdline)
            {
                end = max(end, (uint32_t)mods[i].cmdline + (uint32_t)strlen((char*)mods[i].cmdline) + 1);
            }
        }
    }

    if (mbd->flags & MULTIBOOT_INFO_ELF_SHDR) 
    {
        // ELF section header table (preferred over a.out)
        end = max(end, mbd->u.elf_sec.addr + mbd->u.elf_sec.size);
    } 
    else if (mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) 
    {
        end = max(end, mbd->u.aout_sym.addr + mbd->u.aout_sym.strsize + mbd->u.aout_sym.tabsize);
    }

    if (mbd->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        end = max(end, mbd->mmap_addr + mbd->mmap_length);
    }

    if (mbd->flags & MULTIBOOT_INFO_DRIVE_INFO)
    {
        end = max(end, mbd->drives_addr + mbd->drives_length);
    }

    if (mbd->flags & MULTIBOOT_INFO_APM_TABLE)
    {    
        end = max(end, (uint32_t)mbd->apm_table + (uint32_t)sizeof(struct multiboot_apm_info));
    }
    if (mbd->flags & MULTIBOOT_INFO_VBE_INFO) 
    {
        // Per VBE spec: control info ~512 bytes, mode info ~256 bytes.
        // Many bootloaders place these exact-sized blobs.
        end = max(end, (uint32_t)mbd->vbe_control_info + 512);
        end = max(end, (uint32_t)mbd->vbe_mode_info    + 256);
    }

    return round_page_up(end);
}


typedef struct bump_vars
{
    uint32_t max_addr;
    uint32_t alloc_addr;
    uint32_t begin_addr;
} bump_vars_t;

static bump_vars_t bump;

static void* bump_alloc_align(const uint32_t size, uint32_t alignment)
{
    if (alignment < sizeof(void*)) 
    {
        alignment = sizeof(void*);
    }
    alignment = align_up_pow2(alignment);
    
    uint32_t aligned_addr = (bump.alloc_addr + alignment - 1) & ~(alignment - 1);
    if (aligned_addr > UINT32_MAX - size) 
    { 
        debug_print_str("bump: overflow\n"); 
        cpu_halt(); 
    }
    
    uint32_t needed_end = aligned_addr + size;

    // Allocate more pages
    if (needed_end > bump.max_addr) 
    {
        uint32_t new_max = round_page_up(needed_end); 
        uint32_t delta   = (new_max - bump.max_addr);

        assert(map_pages(
            (void*)bump.max_addr, delta/PAGE_SIZE, 
            PAGETYPE_PHYS_PAGES, 
            PAGEFLAG_KERNEL
        ));

        bump.max_addr = new_max;
    }

    bump.alloc_addr = aligned_addr + size;

    return (void*)aligned_addr;
}

static void init_bump(uint32_t begin_addr)
{
    uint32_t pages_count = round_page_up(INIT_SIZE)/PAGE_SIZE;
    map_pages(
        (void*)begin_addr, pages_count, 
        PAGETYPE_PHYS_PAGES, 
        PAGEFLAG_KERNEL
    );

    bump.alloc_addr = begin_addr;
    bump.begin_addr = begin_addr;
    bump.max_addr = begin_addr + pages_count*PAGE_SIZE;
}

phys_page_descriptor_t* phys_page_descs;
uint32_t pages_count;

phys_page_descriptor_t *phys_to_pfn(void *phys_addr)
{
    uint32_t page_index = (uint32_t)phys_addr/PAGE_SIZE;

    assert(page_index < pages_count);

    return &phys_page_descs[page_index];
}

phys_page_descriptor_t *virt_to_pfn(void *addr)
{
    uint32_t page_index = (uint32_t)get_phys_addr(addr)/PAGE_SIZE;

    assert(page_index < pages_count);

    return &phys_page_descs[page_index];
}

static inline phys_page_descriptor_t* mark_range(uint32_t start_pa, uint32_t end_pa,
                              phys_page_descriptor_t* pages, uint32_t total_pages,
                              enum phys_page_type type, uint32_t ref_count, uint32_t flags)
{
    if (end_pa <= start_pa) 
    {
        return NULL;
    }

    uint32_t start = start_pa >> 12;
    uint32_t end = (end_pa - 1) >> 12; // inclusive
    if (start >= total_pages)
    { 
        return NULL;
    }
    if (end >= total_pages) 
    {
        end = total_pages - 1;
    }

    for (uint32_t i = start; i <= end; ++i) 
    {
        pages[i].type = type;
        pages[i].ref_count = ref_count;
        pages[i].flags = flags;
    }
    
    return &pages[start];
}

// Init descriptor, no impact to real descriptors
phys_page_descriptor_t* page_desc_free_ll;

static void free_page_desc_region(uint32_t start, uint32_t end)
{
    uint32_t page_index     = start / PAGE_SIZE;
    uint32_t page_index_end = end   / PAGE_SIZE;
    uint32_t total_pages    = pages_count; 

    if (page_index >= total_pages) 
    {
        return;
    }
    if (page_index_end > total_pages) 
    {
        page_index_end = total_pages;
    }

    while (page_index < page_index_end)
    {
        while (page_index < page_index_end && phys_page_descs[page_index].type != PAGETYPE_UNUSED)
        {
            page_index++;
        }
    
        if (page_index >= page_index_end)
        {
            break;
        }
        
        uint32_t from_index = page_index;
        phys_page_descriptor_t* head = &phys_page_descs[from_index];

        while (page_index < page_index_end && phys_page_descs[page_index].type == PAGETYPE_UNUSED)
        {
            phys_page_descs[page_index].u.free_page.count = 0;
            phys_page_descs[page_index].u.free_page.prev_desc = head;
            page_index++;
        }

        uint32_t region_count = page_index - from_index;

        phys_page_descriptor_t* foot = &phys_page_descs[from_index + region_count - 1];

        foot->u.free_page.count = region_count;
        head->u.free_page.count = region_count;

        head->u.free_page.next_desc = page_desc_free_ll;
        head->u.free_page.prev_desc = NULL;

        page_desc_free_ll = head;
        
        if (head->u.free_page.next_desc)
        {
            head-> u.free_page.next_desc-> u.free_page.prev_desc = head;
        }
    }
}

void rebuild_free_list()
{
    page_desc_free_ll = NULL;

    uint32_t i = 0;
    while (i < pages_count) 
    {
        while (i < pages_count && phys_page_descs[i].type != PAGETYPE_UNUSED) 
        {
            phys_page_descs[i].u.free_page.count     = 0;
            phys_page_descs[i].u.free_page.prev_desc = NULL;
            phys_page_descs[i].u.free_page.next_desc = NULL;
            ++i;
        }
        
        if (i >= pages_count) 
            break;

        // Start of a free run
        uint32_t start = i;
        while (i < pages_count && phys_page_descs[i].type == PAGETYPE_UNUSED) 
        {
        
            phys_page_descs[i].u.free_page.count = 0;
            phys_page_descs[i].u.free_page.prev_desc = &phys_page_descs[start];
        
            ++i;
        }

        uint32_t cnt = i - start;
        
        if (cnt == 0) 
            continue;

        phys_page_descriptor_t* head = &phys_page_descs[start];
        phys_page_descriptor_t* foot = &phys_page_descs[start + cnt - 1];

        // Fix head/foot
        head->u.free_page.count = cnt;
        foot->u.free_page.count = cnt;

        // push head list 
        head->u.free_page.prev_desc = NULL;
        head->u.free_page.next_desc = page_desc_free_ll;
        if (page_desc_free_ll)
            page_desc_free_ll->u.free_page.prev_desc = head;
        page_desc_free_ll = head;
    }
}

static void mark_paging_structures(phys_page_descriptor_t* phys_pages)
{
    // 1) Mark the page directory frame
    uint32_t pd_phys = (uint32_t)get_phys_addr(&page_directory);
    uint32_t page_index = pd_phys/PAGE_SIZE;
    
    phys_pages[page_index].type = PAGETYPE_DIRECTORY;
    
    for (uint32_t pdi = 0; pdi < ENTRIES_AMOUNT; ++pdi) 
    {
        page_table_t* page_table = get_page_table(pdi);
        if (!page_table)
        {
            continue;
        }

        uint32_t phys_addr = (uint32_t)get_phys_addr(page_table);
        uint32_t page_index = phys_addr/PAGE_SIZE;
        
        phys_pages[page_index].type = PAGETYPE_TABLE;
    }
}

static void init_phys_pages(multiboot_info_t* mbd,
                     phys_page_descriptor_t* phys_pages,
                     uint32_t pages_count)
{
    // Mark everything as RESERVERD
    mark_range(0, pages_count * PAGE_SIZE,
               phys_pages, pages_count, 
               PAGETYPE_RESERVED, 1, 0);


    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    // Make all free areas
    while ((uint32_t)mmap < mmap_end)
    {
        uint32_t base = mmap->addr_low;
        uint32_t len  = mmap->len_low;
        
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            mark_range(
                round_page_up(base), 
                round_page_down(base + len), 
                phys_pages, pages_count,
                PAGETYPE_UNUSED, 0,
                PAGEFLAG_KERNEL | PAGEFLAG_VFREE
            );
        }
        else if (mmap->type == MULTIBOOT_MEMORY_RESERVED) 
        {
            mark_range(
                round_page_down(base), 
                round_page_up(base + len), 
                phys_pages, pages_count,
                PAGETYPE_RESERVED, 1,
                PAGEFLAG_KERNEL | PAGEFLAG_READONLY
            );
        }

        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // Kernel image
    mark_range(round_page_down(kernel_begin_pa),
               round_page_up(kernel_end_pa),
               phys_pages, pages_count, 
               PAGETYPE_KERNEL, 1, 0);

    // Multiboot info + modules if any
    mark_range(round_page_down((uint32_t)mbd),
               round_page_up((uint32_t)multiboot_data_end_pa(mbd)),
               phys_pages, pages_count, 
               PAGETYPE_RESERVED, 1, 0);

    // Phys page descriptors backing storage
    mark_range(round_page_down((uint32_t)get_phys_addr((void*)bump.begin_addr)),
               round_page_up((uint32_t)get_phys_addr((void*)bump.alloc_addr)),
               phys_pages, pages_count, 
               PAGETYPE_KERNEL, 1, 0);

    mark_paging_structures(phys_pages);
    
    // Make the free descriptor list
    page_desc_free_ll = NULL;
    mmap = (multiboot_memory_map_t*)mbd->mmap_addr;
    mmap_end = mbd->mmap_addr + mbd->mmap_length;
    while ((uint32_t)mmap < mmap_end)
    {
        uint32_t base = mmap->addr_low;
        uint32_t len  = mmap->len_low;
        
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            free_page_desc_region(round_page_up(base), round_page_down(base+len));
        }
        else
        {
            uint64_t begin = round_page_down(base);
            uint64_t end = round_page_up( begin + len );

            identity_map_pages(
                (void*)begin, (end-begin)/PAGE_SIZE, 
                PAGETYPE_RESERVED, 
                PAGEFLAG_KERNEL | PAGEFLAG_READONLY
            );
        }

        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    rebuild_free_list();
}

void* alloc_phys_page_pfn(enum phys_page_type page_type, uint32_t page_flags)
{
    if (!page_desc_free_ll)
    {
        return NULL;
    }

    if (page_type == PAGETYPE_UNUSED)
    {
        return NULL;
    }

    uint32_t page_index = page_desc_free_ll - phys_page_descs;
    
    if (page_desc_free_ll->u.free_page.count > 1)
    {
        uint32_t end_page_index = page_index + page_desc_free_ll->u.free_page.count - 1;

        phys_page_descriptor_t* res_page_desc = &phys_page_descs[end_page_index];
        res_page_desc->type = page_type;
        res_page_desc->flags = page_flags;
        res_page_desc->ref_count = 1;

        page_desc_free_ll->u.free_page.count--;

        uint32_t new_foot_page_index = end_page_index - 1;
        phys_page_descriptor_t* new_foot = &phys_page_descs[new_foot_page_index];
        new_foot->u.free_page.count = page_desc_free_ll->u.free_page.count;

        return (void*)(end_page_index  * PAGE_SIZE);
    }

    phys_page_descriptor_t* next = page_desc_free_ll->u.free_page.next_desc;
    page_desc_free_ll = next;
    if (next)
    {
        next->u.free_page.prev_desc = NULL;
    }

    phys_page_descriptor_t* res_page_desc =&phys_page_descs[page_index];
    res_page_desc->type = page_type;
    res_page_desc->flags = page_flags;
    res_page_desc->ref_count = 1;

    return (void*)(page_index * PAGE_SIZE);
}

void free_phys_page_pfn(void* addr_ptr)
{   
    uint32_t addr = (uint32_t)addr_ptr;

    assert((addr & (PAGE_SIZE - 1)) == 0);
    assert(addr < max_memory);

    uint32_t page_index = addr>>12;
    phys_page_descriptor_t* cur_desc = &phys_page_descs[page_index];
    
    assert(cur_desc->type != PAGETYPE_UNUSED);
    assert(cur_desc->ref_count > 0);

    if (--cur_desc->ref_count > 0) 
        return; 

    cur_desc->type = PAGETYPE_UNUSED;

    phys_page_descriptor_t* old_foot = (page_index > 0) ? 
            &phys_page_descs[page_index-1] : NULL;
    if (old_foot && old_foot->type != PAGETYPE_UNUSED)
        old_foot = NULL;

    phys_page_descriptor_t* old_head = (page_index + 1 < pages_count) ? 
            &phys_page_descs[page_index+1] : NULL;
    if (old_head && old_head->type != PAGETYPE_UNUSED)
        old_head = NULL;

    // back desc is foot
    if (old_foot && !old_head)
    {
        // back desc is head
        phys_page_descriptor_t* new_head = &phys_page_descs[page_index - old_foot->u.free_page.count];
        phys_page_descriptor_t* new_foot = cur_desc;

        cur_desc->u.free_page.prev_desc = NULL;
        cur_desc->u.free_page.next_desc = NULL;

        uint32_t new_count = new_head->u.free_page.count + 1;
        
        new_head->u.free_page.count = new_count;
        new_foot->u.free_page.count = new_count;

        return;
    }
    // next desc is head
    else if (!old_foot && old_head)
    {
        phys_page_descriptor_t* new_foot = &phys_page_descs[page_index + old_head->u.free_page.count];
        phys_page_descriptor_t* new_head = cur_desc;

        uint32_t new_count = old_head->u.free_page.count + 1;

        new_foot->u.free_page.count = new_count; 
        new_head->u.free_page.count = new_count;

        phys_page_descriptor_t* next = old_head->u.free_page.next_desc;
        phys_page_descriptor_t* prev = old_head->u.free_page.prev_desc;

        new_head->u.free_page.next_desc = next;
        new_head->u.free_page.prev_desc = prev;

        if (next)
            next->u.free_page.prev_desc = new_head;

        if (prev)
            prev->u.free_page.next_desc = new_head;
        else
            page_desc_free_ll = new_head;

        return;
    }
    // neither is head or foot
    else if (!old_foot && !old_head)
    {
        cur_desc->u.free_page.count = 1;
        
        cur_desc->u.free_page.prev_desc = NULL;
        cur_desc->u.free_page.next_desc = page_desc_free_ll;

        if (page_desc_free_ll)
            page_desc_free_ll->u.free_page.prev_desc = cur_desc;

        page_desc_free_ll = cur_desc;

        return;
    }

    // there is a head and foot
    
    phys_page_descriptor_t* new_head = 
        &phys_page_descs[page_index - old_foot->u.free_page.count];

    phys_page_descriptor_t* new_foot = 
        &phys_page_descs[page_index + old_head->u.free_page.count];

    // update count
    uint32_t new_count = old_foot->u.free_page.count + old_head->u.free_page.count + 1; 
    new_head->u.free_page.count = new_count;
    new_foot->u.free_page.count = new_count;

    // remove old_head from the list
    phys_page_descriptor_t* old_head_prev = old_head->u.free_page.prev_desc;
    phys_page_descriptor_t* old_head_next = old_head->u.free_page.next_desc;
 
    if (old_head_prev)
    {
        old_head_prev->u.free_page.next_desc = old_head_next;
    }
    else
    {
        page_desc_free_ll = old_head_next;
    }

    if (old_head_next)
    {
        old_head_next->u.free_page.prev_desc = old_head_prev;
    }
}

uint32_t setup_page_descriptors(uint32_t alloc_addr, multiboot_info_t* mbd)
{
    init_bump(alloc_addr);

    pages_count = max_memory / PAGE_SIZE;
    phys_page_descs = bump_alloc_align(
        pages_count * sizeof(phys_page_descriptor_t), 
        0
    );

    memset(phys_page_descs, 0, pages_count * sizeof(phys_page_descriptor_t));

    init_phys_pages(mbd, phys_page_descs, pages_count);

    return bump.alloc_addr;
}