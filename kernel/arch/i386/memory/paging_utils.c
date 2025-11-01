#include <arch/i386/core/paging.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/core/pfn_desc.h>
#include <memory/core/memory_manager.h>
#include <arch/i386/memory/paging_utils.h>
#include <string.h>
#include <kernel/memory/paging.h>


u32 get_pde_index(void *va)
{
    return (usize_ptr)va >> 22;
}

u32 get_pte_index(void *va)
{
    return ((usize_ptr)va >> 12) & 0x3FF;
}

bool map_table_entry(void* phys_addr_ptr, void* virt_addr_ptr, u16 flags)
{
    u32 phys_addr = (u32)phys_addr_ptr;

    // Calculate indices
    u32 page_dir_index = get_pde_index(virt_addr_ptr);

    page_directory_t* page_directory = (page_directory_t*)0xFFFFF000;
    if ((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT)
        return false;
    
    page_directory->entries[page_dir_index] = (void*)( (phys_addr & PAGE_MASK) | (u32)flags );
    
    memset((void*)(0xFFC00000 + page_dir_index*PAGE_SIZE), 0, PAGE_SIZE);

    return true;
}

bool map_page_entry(void* phys_addr_ptr, void* virt_addr_ptr, u16 flags)
{
    u32 phys_addr = (u32)phys_addr_ptr;

    // Calculate indices
    u32 page_dir_index = get_pde_index(virt_addr_ptr);
    u32 page_table_index = get_pte_index(virt_addr_ptr);

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return false;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * PAGE_SIZE));
    page_table->entries[page_table_index] = (void*)((phys_addr & 0xFFFFF000) | ((u32)flags & 0x00000FFF));

    invlpg(virt_addr_ptr);

    return true;
}

page_table_t *get_page_table(u32 dir_index)
{
    page_directory_t* page_dir = (page_directory_t*)0xFFFFF000;
    u32 page_dir_entry = (u32)page_dir->entries[dir_index];
    
    if ((page_dir_entry & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    return (page_table_t*)(0xFFC00000 + dir_index * PAGE_SIZE);
}
void* get_page_phys_base(page_table_t *table, u32 index)
{
    if (((u32)table->entries[index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
        return NULL;

    return (void*) (
        ((u32)table->entries[index] & ~0xFFF) 
    );
}
void* virt_to_phys(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;

    // Calculate indices
    u32 page_dir_index = (virt_addr) >> 22;
    u32 page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    if (((u32)page_table->entries[page_table_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    return (void*) (
        ((u32)page_table->entries[page_table_index] & ~0xFFF) | (virt_addr & 0xFFF) 
    );
}

void set_page_entry(void *virt_addr, u32 entry) 
{
    u32 vaddr = (u32)virt_addr;

    u32 page_dir_index   = vaddr >> 22;
    u32 page_table_index = (vaddr >> 12) & 0x3FF;

    page_directory_t *page_directory = (void*)0xFFFFF000;

    if (((u32) page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0) 
    {
        return;
    }

    page_table_t *page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * PAGE_SIZE));

    page_table->entries[page_table_index] = (void*)entry;

    asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

u32 get_phys_flags(void *va) {
  u32 result = 0;

  u32 entry = get_page_entry(va);
  if (entry & (PAGE_ENTRY_FLAG_PRESENT))
    result |= VIRT_PHYS_FLAG_PRESENT;

  if (entry & PAGE_ENTRY_FLAG_DIRTY)
    result |= VIRT_PHYS_FLAG_DIRTY;

  if (entry & PAGE_ENTRY_FLAG_ACCESS)
    result |= VIRT_PHYS_FLAG_ACCESS;

  if (entry & PAGE_ENTRY_FLAG_WRITE)
    result |= VIRT_PHYS_FLAG_WRITE;

  if (entry & PAGE_ENTRY_FLAG_USER)
    result |= VIRT_PHYS_FLAG_USER;

  return result;
}

void clear_phys_flags(void *va, u32 flags) 
{
    u32 entry = get_page_entry(va);
    if (flags & VIRT_PHYS_FLAG_PRESENT)
        abort();

    if (entry & VIRT_PHYS_FLAG_ACCESS)
        entry &= ~PAGE_ENTRY_FLAG_ACCESS;

    if (entry & VIRT_PHYS_FLAG_DIRTY)
        entry &= ~PAGE_ENTRY_FLAG_DIRTY;

    if (entry & VIRT_PHYS_FLAG_WRITE)
        entry &= ~PAGE_ENTRY_FLAG_WRITE;

    if (entry & VIRT_PHYS_FLAG_USER)
        entry &= ~PAGE_ENTRY_FLAG_USER;
    
    set_page_entry(va, entry);
}

u32 get_table_entry(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;

    // Calculate index
    u32 page_dir_index = (virt_addr) >> 22;

    page_directory_t* page_directory = (void*)0xFFFFF000;  

    return (u32)page_directory->entries[page_dir_index];
}
u32 get_page_entry(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;
    // Calculate indices
    u32 page_dir_index = (virt_addr) >> 22;
    u32 page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return INVALID_PAGE_MEMORY;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    return (u32)page_table->entries[page_table_index];
}

u16 pfn_to_hw_flags(u16 flags)
{
    u16 result = PAGE_ENTRY_FLAG_PRESENT;
    if ((flags & PAGEFLAG_KERNEL) == false)
    {
        result |= PAGE_ENTRY_FLAG_USER;
    }
    if ((flags & PAGEFLAG_READONLY) == false)
    {
        result |= PAGE_ENTRY_FLAG_WRITE;
    }
    if ((flags & PAGEFLAG_NOEXEC) == false)
    {
        // No NX bit in 32 bit flags...
    }
    
    return result;
}