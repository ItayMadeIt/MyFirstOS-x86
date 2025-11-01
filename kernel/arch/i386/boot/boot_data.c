#include <arch/i386/memory/paging_utils.h>
#include <kernel/core/paging.h>
#include <core/defs.h>
#include <arch/i386/boot/entry_data.h>
#include <kernel/boot/boot_data.h> 
#include <arch/i386/firmware/multiboot/multiboot.h>
#include <kernel/boot/boot_data.h>

bool boot_has_memory(const boot_data_t* boot_data)
{
    multiboot_info_t* mbd = boot_data->mbd;
    return mbd->flags & MULTIBOOT_INFO_MEM_MAP;
}

usize_ptr get_max_memory(const boot_data_t* boot_data)
{
    u32 max_memory = 0;
    multiboot_info_t* mbd = (multiboot_info_t*)boot_data->mbd;

    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) mbd->mmap_addr;
    u32 mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((u32) mmap < mmap_end)
    {
        max_memory = max(
            max_memory,
            round_page_up( mmap->addr_low + mmap->len_low) 
        );

        // Get next iteration
        u32 next_mmap_addr = (u32) mmap + (sizeof(mmap->size) + mmap->size);
        mmap = (multiboot_memory_map_t*) (next_mmap_addr);
    }

    return (usize_ptr)max_memory;
}

static inline const multiboot_memory_map_t* next_mmap(const multiboot_memory_map_t* mmap) 
{
    return (const multiboot_memory_map_t*)((u32)mmap + sizeof(mmap->size) + mmap->size);
}


void boot_foreach_free_page(const boot_data_t* boot_data, void(*callback)(void* pa))
{
    const multiboot_info_t* mbd = (const multiboot_info_t*)boot_data->mbd;

    const multiboot_memory_map_t* mmap = (const multiboot_memory_map_t*) mbd->mmap_addr;
    u32 mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((u32) mmap < mmap_end)
    {
        u32 low = round_page_up(mmap->addr_low);
        u32 high = round_page_down(mmap->addr_low + mmap->len_low);

        if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE ||
            low >= high)
        {
            // Get next iteration
            mmap = next_mmap(mmap);
            continue;
        }

        // Only area > 1MiB
        if (high <= STOR_1MiB) 
        {
            mmap = next_mmap(mmap);
            continue;
        }

        // Clamp (1MiB, high)
        if (low < STOR_1MiB)
        {
            low = STOR_1MiB;
        }

        while (low < high)
        {
            callback((void*)low);
            low += PAGE_SIZE;
        }

        // Get next iteration
        mmap = next_mmap(mmap);
    }
}

void boot_foreach_free_page_region(const boot_data_t* boot_data, void(*callback)(void* from_pa,void* to_pa))
{
    const multiboot_info_t* mbd = (const multiboot_info_t*)boot_data->mbd;

    const multiboot_memory_map_t* mmap = (const multiboot_memory_map_t*) mbd->mmap_addr;
    u32 mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((u32) mmap < mmap_end)
    {
        u32 low  = round_page_up(mmap->addr_low);
        u32 high = round_page_down(mmap->addr_low + mmap->len_low);

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE &&
            low < high)
        {
            callback((void*)low, (void*)high);
        }
        
        // Get next iteration
        mmap = next_mmap(mmap);
    }
}

void boot_foreach_reserved_region(const boot_data_t* boot_data,
                                  void(*callback)(void* from_pa, void* to_pa))
{
    const multiboot_info_t* mbd = (const multiboot_info_t*)boot_data->mbd;

    const multiboot_memory_map_t* mmap = (const multiboot_memory_map_t*) mbd->mmap_addr;
    u32 mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((u32)mmap < mmap_end)
    {
        u32 low  = round_page_down(mmap->addr_low);
        u32 high = round_page_up(mmap->addr_low + mmap->len_low);

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE || low >= high)
        {
            mmap = next_mmap(mmap);
            continue;
        }
        if (high <= STOR_1MiB)
        {
            mmap = next_mmap(mmap);
            continue;
        }

        if (low < STOR_1MiB)
            callback((void*)STOR_1MiB, (void*)high);
        else 
            callback((void*)low, (void*)high);

        mmap = next_mmap(mmap);
    }

    callback((void*)0, (void*)STOR_1MiB);
}

void boot_foreach_page_struct(void(*callback)(void* pa))
{
    usize_ptr pd_pa = (usize_ptr)virt_to_phys(&page_directory);
    callback((void*)pd_pa);
    
    for (u32 pdi = 0; pdi < ENTRIES_AMOUNT; ++pdi) 
    {
        page_table_t* pt = get_page_table(pdi);
        if (!pt)
        {
            continue;
        }

        u32 pt_pa = (u32)virt_to_phys(pt);
        callback((void*)pt_pa);
    }
}