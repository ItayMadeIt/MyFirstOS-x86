#include <core/defs.h>
#include <core/paging.h>
#include <memory/virt_alloc.h>
#include <memory/phys_alloc.h>
#include <memory/heap.h>

#define EARLY_HEAP_VIRT 0xE0000000
#define HEAP_VIRT 0xD0000000
#define BUDDY_SIZES_COUNT 17 /*2^12 to 2^28 (no need to more than 256MB at once) */
#define BUDDY_MIN_SIZE 4096 /*2^12*/
#define BUDDY_MAX_EXP 28 /*2^28*/ 
#define BUDDY_MIN_EXP 12 /*2^12*/
#define BUDDY_MIN_CAPICTY (8 * sizeof(buddy_alloc_block_t))
#define SLAB_SIZES_COUNT 8 /*2^4 to 2^11 (less than 4096 bytes and at least 16 bytes) */
#define SLAB_MIN_INDEX 4 /*2^12*/
#define SLAB_MIN_CAPICTY (8 * sizeof(slab_alloc_block_t))

buddy_alloc_blocks_t buddy_blocks[BUDDY_SIZES_COUNT];
slab_alloc_blocks_t slab_blocks[SLAB_SIZES_COUNT];

page_table_t early_heap_table;
uint32_t early_heap_max_addr;

static uint32_t early_alloc_addr = EARLY_HEAP_VIRT;

extern uint8_t pages_allocated[PAGES_ARR_SIZE];
extern uint32_t max_memory;

page_t* pages;

static void* early_alloc_align(const uint32_t size, const uint32_t alignment/*aligment 2^n*/)
{
    uint32_t aligned_addr = (early_alloc_addr + alignment - 1) & ~(alignment - 1);

    // Allocate more pages
    while (aligned_addr + size + STOR_4Kib > early_heap_max_addr)
    {    
        uint32_t aligned_table_addr = (early_alloc_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        page_table_t* new_table = (page_table_t*)(aligned_table_addr);

        alloc_table(get_phys_addr(new_table), early_heap_max_addr, PAGE_ENTRY_WRITE_KERNEL_FLAGS);

        early_heap_max_addr += STOR_4MiB;
        early_alloc_addr = aligned_table_addr + PAGE_SIZE;
    
        aligned_addr = (early_alloc_addr + alignment - 1) & ~(alignment - 1);
    }

    early_alloc_addr = aligned_addr + size;

    return (void*)aligned_addr;
}

static void* early_alloc(uint32_t size)
{
    return early_alloc_align(size, 1/*One byte alignment = no aligment*/);
}

static void init_buddy_alloc_arr()
{
    uint32_t cur_size = BUDDY_MIN_SIZE;

    for (uint32_t i = 0; i < BUDDY_SIZES_COUNT; i++)
    {
        buddy_blocks[i].buddy_size = cur_size;
        cur_size <<= 1; // *= 2

        buddy_blocks[i].head = NULL;
    }
}

static void init_slab_alloc_arr()
{
    uint32_t cur_size = SLAB_MIN_SIZE;

    for (uint32_t i = 0; i < SLAB_SIZES_COUNT; i++)
    {
        slab_blocks[i].slab_size = cur_size;
        cur_size <<= 1; // *= 2

        slab_blocks[i].head = NULL;
    }
}

static void* (*kmalloc)(const uint32_t size);
static void (*kfree)(const uint32_t addr);

// insert_buddy doesn't merge, just sorted insert
static void insert_buddy(uint32_t addr, uint32_t buddy_size /*Must be 2^n*/)
{
    // checks if buddy size is the same as the lsb alone
    if (buddy_size != (buddy_size & (-buddy_size))) 
    {
        debug_print_str("Invalid buddy size");
        halt();
    }

    uint32_t index = log2_u32(buddy_size) - BUDDY_MIN_EXP;

    buddy_node_t** link = &buddy_blocks[index].head;
    buddy_node_t* lst = buddy_blocks[index].head;
    while (lst && lst->addr < addr)
    {
        link = &lst->next;
        lst = lst->next;
    }

    buddy_node_t* new_buddy = 
        (buddy_node_t*)kmalloc(sizeof(buddy_node_t));

    new_buddy->next = *link;
    new_buddy->addr = addr;
    *link = new_buddy;
    
}


// True if merged, false if not
static bool merge_iteration(uint32_t size_index, uint32_t max_addr)
{
    if (buddy_blocks[size_index].head == NULL) // empty list
    {
        return false;
    }

    // Set up traversal pointers: prev -> current
    buddy_node_t** prev_link = &buddy_blocks[size_index].head;
    buddy_node_t** cur_link = &(buddy_blocks[size_index].head->next);
    buddy_node_t* cur = *cur_link;

    max_addr += buddy_blocks[size_index].buddy_size;

    // Iterate
    while (cur)
    {
        uint32_t prev_addr = (*prev_link)->addr;
        uint32_t cur_addr = cur->addr;

        // No merge possible
        if (cur_addr > max_addr)
        {
            return false;
        }

        // Can't merge, advance
        if ((prev_addr ^ cur_addr) != buddy_blocks[size_index].buddy_size)
        {

            prev_link = cur_link;
            cur_link = &cur->next;
            cur = *cur_link;
            continue;
        }

        // Merge
        buddy_node_t* prev = *prev_link;
        *prev_link = cur->next;

        kfree(prev);
        kfree(cur);

        insert_buddy(prev_addr, buddy_blocks[size_index].buddy_size * 2);

        return true;   
    }

    return false;
}

static void merge_buddies(uint32_t size_index, uint32_t max_addr)
{
    while (size_index <= BUDDY_MAX_EXP) // Can't merge at BUDDY_MAX_EXP
    {
        if (merge_iteration(size_index, max_addr) == false)
        {
            break;
        }

        max_addr += buddy_blocks[size_index].buddy_size;

        ++size_index;
    }
}

void set_phys_pages_struct_type(uint32_t from_virt, uint32_t to_virt /*non inclusive*/, uint32_t struct_addr, uint8_t type)
{
    from_virt = from_virt & ~0xFFF;
    to_virt = to_virt & ~0xFFF;

    for (uint32_t virt = from_virt & ~0xFFF; virt < to_virt; virt += PAGE_SIZE)
    {
        uint32_t phys = get_phys_addr(virt);
        if (phys == INVALID_PAGE_MEMORY)
        { 
            continue;
        }

        uint32_t phys_index = phys >> 12;
        pages[phys_index].type = type;
        pages[phys_index].struct_addr = struct_addr;
    }
}

uint32_t allocate_buddy_size(uint32_t size)
{
    if (size < PAGE_SIZE)
    {
        debug_print_str("Can't ask for size lower than page size");
        halt();
    }

    size = align_pow2(size);

    uint32_t index = log2_u32(size) - BUDDY_MIN_EXP;

    // Allocate buddy block of the fitting size
    if (buddy_blocks[index].head)
    {
        buddy_node_t* node = buddy_blocks[index].head;
        buddy_blocks[index].head = node->next;
        
        uint32_t addr = node->addr;

        page_buddy_metadata_t* page_metadata = (page_buddy_metadata_t*)kmalloc(sizeof(page_buddy_metadata_t));
        page_metadata->buddy_node = NULL;
        page_metadata->buddy_size = buddy_blocks[index].buddy_size;
        
        set_phys_pages_struct_type(addr, addr+size, page_metadata, page_type_heap_buddy);

        return addr;
    }

    uint32_t original_index = index;

    // Find bigger size that can be utilized
    do
    {
        ++index;
    }
    while (buddy_blocks[index].head == NULL);


    // buddy_blocks[index].head != NULL, split by 2 until fitting size
    while (index > original_index)
    {
        // Copy 1 node to lower and allocate another 1
        buddy_node_t* left_node = buddy_blocks[index].head;
        
        buddy_blocks[index].head = left_node->next;
    
        --index;

        buddy_node_t* right_node = (buddy_node_t*)kmalloc(sizeof(buddy_node_t));
        right_node->addr = left_node->addr + buddy_blocks[index].buddy_size;
        right_node->next = NULL;

        left_node->next = right_node;

        buddy_blocks[index].head = left_node;
    }

    buddy_node_t* node = buddy_blocks[index].head;
    buddy_blocks[index].head = node->next;
    
    uint32_t addr = node->addr;

    page_buddy_metadata_t* page_metadata = (page_buddy_metadata_t*)kmalloc(sizeof(page_buddy_metadata_t));
    page_metadata->buddy_node = NULL;
    page_metadata->buddy_size = buddy_blocks[index].buddy_size;
    
    set_phys_pages_struct_type(addr, addr+size, page_metadata, page_type_heap_buddy);

    return addr;
}

void free_buddy_alloc(uint32_t addr)
{
    uint32_t phys_addr = get_phys_addr(addr);
    uint32_t phys_page_index = phys_addr>>12;


    if (pages[phys_page_index].type != page_type_heap_buddy)
    {
        debug_print_str("Must be buddy\n");
        debug_print(addr);
        debug_print(phys_addr);
        debug_print(phys_page_index);
        debug_print(pages[phys_page_index].type);
        halt();
    }
    page_buddy_metadata_t* buddy_metadata = (page_buddy_metadata_t*)pages[phys_page_index].struct_addr;

    pages[phys_page_index].type = page_type_heap;
    pages[phys_page_index].struct_addr = NULL;

    uint32_t buddy_blocks_index = log2_u32(buddy_metadata->buddy_size) - BUDDY_MIN_EXP;

    insert_buddy(addr, buddy_metadata->buddy_size);

    merge_buddies(buddy_blocks_index, addr);

    kfree(buddy_metadata);
}

uint32_t buddy_block_length(buddy_node_t* node)
{
    uint32_t size = 0;
    while (node)
    {
        ++size;
        node = node->next;
    }
    return size;
}

void init_early_heap()
{
    // Alloc 4mb for early heap
    alloc_table(get_phys_addr(&early_heap_table), EARLY_HEAP_VIRT, PAGE_ENTRY_WRITE_KERNEL_FLAGS);
    early_heap_max_addr = EARLY_HEAP_VIRT + STOR_4MiB;
}

void init_heap()
{
    // Alloc 4mb for actual heap
    page_table_t* heap_table = early_alloc_align(sizeof(page_table_t),  PAGE_SIZE);
    alloc_table(get_phys_addr(heap_table), HEAP_VIRT, PAGE_ENTRY_WRITE_KERNEL_FLAGS);    

    set_phys_pages_struct_type(HEAP_VIRT, HEAP_VIRT + STOR_4MiB, NULL, page_type_heap);
}

static void free_phys_pages_from_bitmap(uint8_t bitmap, uint32_t byte_index)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        if (bitmap & (1 << i))
        {
            uint32_t phys_page_index = byte_index * BIT_TO_BYTE + i;
            pages[phys_page_index].type = page_type_available;
        }
    }
}

static void setup_phys_pages()
{
    uint32_t pages_count = max_memory >> 12;

    pages = (page_t*)early_alloc_align(pages_count * sizeof(page_t), sizeof(uint32_t));

    for (uint32_t i = 0; i < pages_count; i++)
    {
        pages[i].struct_addr = NULL;
        pages[i].extra = 0;
        pages[i].flags = 0;
        pages[i].type = page_type_kernel_static; // Make all unavailable
    }

    // Free all available pages
    for (uint32_t page_index = 0; page_index < pages_count; page_index++)
    {
        free_phys_pages_from_bitmap(pages_allocated[page_index], page_index);
    }
}

void empty_free(uint32_t size){}

void setup_heap()
{
    pages = NULL;
    init_early_heap();
    setup_phys_pages();

    init_heap();

    init_buddy_alloc_arr();
    init_slab_alloc_arr();

    kmalloc = early_alloc;
    kfree = empty_free;

    insert_buddy(HEAP_VIRT, STOR_4MiB);

    uint32_t new_addr = allocate_buddy_size(4096);
    uint32_t new_addr3 = allocate_buddy_size(8128);
    uint32_t new_addr2 = allocate_buddy_size(4096);
    debug_print(new_addr);
    debug_print(new_addr2);
    debug_print(new_addr3);

    free_buddy_alloc(new_addr);
    free_buddy_alloc(new_addr2);
    free_buddy_alloc(new_addr3);


    new_addr2 = allocate_buddy_size(8128);
    debug_print(new_addr2);

    new_addr3 = allocate_buddy_size(8128);
    debug_print(new_addr3);

    free_buddy_alloc(new_addr2);
    free_buddy_alloc(new_addr3);
    
    halt();
    
}