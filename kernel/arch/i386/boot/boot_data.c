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

uintptr_t get_max_memory(const boot_data_t* boot_data)
{
    uint32_t max_memory = 0;
    multiboot_info_t* mbd = (multiboot_info_t*)boot_data->mbd;

    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t) mmap < mmap_end)
    {
        max_memory = max(
            max_memory,
            round_page_up( mmap->addr_low + mmap->len_low) 
        );

        // Get next iteration
        uint32_t next_mmap_addr = (uint32_t) mmap + (sizeof(mmap->size) + mmap->size);
        mmap = (multiboot_memory_map_t*) (next_mmap_addr);
    }

    return (uintptr_t)max_memory;
}

static inline const multiboot_memory_map_t* next_mmap(const multiboot_memory_map_t* mmap) 
{
    return (const multiboot_memory_map_t*)((uint32_t)mmap + sizeof(mmap->size) + mmap->size);
}


void boot_foreach_free_page(const boot_data_t* boot_data, void(*callback)(void* pa))
{
    const multiboot_info_t* mbd = (const multiboot_info_t*)boot_data->mbd;

    const multiboot_memory_map_t* mmap = (const multiboot_memory_map_t*) mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t) mmap < mmap_end)
    {
        uint32_t low = round_page_up(mmap->addr_low);
        uint32_t high = round_page_down(mmap->addr_low + mmap->len_low);

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
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t) mmap < mmap_end)
    {
        uint32_t low  = round_page_up(mmap->addr_low);
        uint32_t high = round_page_down(mmap->addr_low + mmap->len_low);

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
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while ((uint32_t)mmap < mmap_end)
    {
        uint32_t low  = round_page_down(mmap->addr_low);
        uint32_t high = round_page_up(mmap->addr_low + mmap->len_low);

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
    uintptr_t pd_pa = (uintptr_t)virt_to_phys(&page_directory);
    callback((void*)pd_pa);
    
    for (uint32_t pdi = 0; pdi < ENTRIES_AMOUNT; ++pdi) 
    {
        page_table_t* pt = get_page_table(pdi);
        if (!pt)
        {
            continue;
        }

        uint32_t pt_pa = (uint32_t)virt_to_phys(pt);
        callback((void*)pt_pa);
    }
}