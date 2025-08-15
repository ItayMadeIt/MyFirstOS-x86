// #include <core/defs.h>
// #include <core/paging.h>
// #include <memory/virt_alloc.h>
// #include <memory/phys_alloc.h>
#include <memory/heap.h>

// #include <core/debug.h>

// #define EARLY_HEAP_VIRT 0xE0000000
// #define HEAP_VIRT 0xD0000000
// #define BUDDY_ORDER_COUNT 17 /*2^12 to 2^28 (no need to more than 256MB at once) */
// #define BUDDY_BLOCK_SIZE_MIN 4096 /*2^12*/
// #define BUDDY_ORDER_MAX 28 /*2^28*/ 
// #define BUDDY_ORDER_MIN 12 /*2^12*/
// #define BUDDY_BLOCK_CAPACITY_MIN (8 * sizeof(buddy_alloc_block_t))
// #define SLAB_ORDER_COUNT 8 /*2^4 to 2^11 (less than 4096 bytes and at least 16 bytes) */
// #define SLAB_ORDER_MIN 4 /*2^4*/
// #define SLAB_ORDER_MAX 11 /*2^11*/
// #define SLAB_BLOCK_CAPACITY_MIN (8 * sizeof(slab_alloc_block_t))

typedef struct slab_node
{
    uint32_t addr;
    uint16_t free_bits;    
    uint8_t bitmap[BITMAP_LEN];
    
    struct slab_node* next;

} slab_node_t;

typedef struct page_slab_metadata
{
    slab_node_t* slab_node; // null = isn't allocated
    uint16_t slab_size; 
    uint16_t unused; 

} page_slab_metadata_t;

typedef struct slab_alloc_blocks
{
    uint32_t slab_size; // 2^n

    slab_node_t* head;

} slab_alloc_blocks_t;

typedef struct buddy_node
{
    uint32_t addr; // page aligned so first few bits are used
    struct buddy_node* next;  
} buddy_node_t;

typedef struct page_buddy_metadata
{
    uint32_t buddy_size;
    buddy_node_t* buddy_node;
} page_buddy_metadata_t __attribute__((aligned(sizeof(uint32_t))));

typedef struct buddy_alloc_blocks
{
    // buddy attr
    uint32_t buddy_size; // 2^n

    buddy_node_t* head;

} buddy_alloc_blocks_t;

typedef struct heap_metadata
{
    uint32_t size;
} heap_metadata_t;

// phys_page_t* pages;

// buddy_alloc_blocks_t buddy_blocks[BUDDY_ORDER_COUNT];
// slab_alloc_blocks_t slab_blocks[SLAB_ORDER_COUNT];

// page_table_t early_heap_table;
// uint32_t early_heap_max_addr;

// static uint32_t early_alloc_addr = EARLY_HEAP_VIRT;

// extern uint8_t pages_allocated[PAGES_ARR_SIZE];
// extern uint32_t max_memory;


// static uint32_t buddy_block_length(buddy_node_t* node)
// {
//     uint32_t size = 0;
//     while (node)
//     {
//         ++size;
//         node = node->next;
//     }
//     return size;
// }

// static inline uint32_t calc_slab_index_size(uint32_t size) 
// {
//     uint32_t aligned = align_pow2(size);
//     return align_pow2(size) <= (1ull << SLAB_ORDER_MIN) ? 0 : log2_u32(aligned) - SLAB_ORDER_MIN;
// };
// static inline uint32_t calc_buddy_index_size(uint32_t size) 
// {
//     uint32_t aligned = align_pow2(size);
//     return align_pow2(size) <= (1ull << BUDDY_ORDER_MIN) ? 0 : log2_u32(aligned) - BUDDY_ORDER_MIN;
// }

// static void* early_alloc_align(const uint32_t size, const uint32_t alignment/*aligment 2^n*/)
// {
//     uint32_t aligned_addr = (early_alloc_addr + alignment - 1) & ~(alignment - 1);

//     // Allocate more pages
//     while (aligned_addr + size + STOR_4KiB > early_heap_max_addr)
//     {    
//         uint32_t aligned_table_addr = (early_alloc_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
//         page_table_t* new_table = (page_table_t*)(aligned_table_addr);

//         alloc_table(get_phys_addr(new_table), (void*)early_heap_max_addr, PAGE_ENTRY_WRITE_KERNEL_FLAGS);

//         early_heap_max_addr += STOR_4MiB;
//         early_alloc_addr = aligned_table_addr + PAGE_SIZE;
    
//         aligned_addr = (early_alloc_addr + alignment - 1) & ~(alignment - 1);
//     }

//     early_alloc_addr = aligned_addr + size;

//     return (void*)aligned_addr;
// }

// static void* early_alloc(uint32_t size)
// {
//     return early_alloc_align(size, sizeof(uint32_t)/*Must be because some things depend*/);
// }

// static void init_buddy_alloc_arr()
// {
//     uint32_t cur_size = BUDDY_BLOCK_SIZE_MIN;

//     for (uint32_t i = 0; i < BUDDY_ORDER_COUNT; i++)
//     {
//         buddy_blocks[i].buddy_size = cur_size;
//         cur_size <<= 1; // *= 2

//         buddy_blocks[i].head = NULL;
//     }
// }

// static void init_slab_alloc_arr()
// {
//     uint32_t cur_size = SLAB_MIN_SIZE;

//     for (uint32_t i = 0; i < SLAB_ORDER_COUNT; i++)
//     {
//         slab_blocks[i].slab_size = cur_size;
//         cur_size <<= 1; // *= 2

//         slab_blocks[i].head = NULL;
//     }
// }

// static void* (*kmalloc)(const uint32_t size);
// static void (*kfree)(const uint32_t addr);

// // insert_buddy doesn't merge, just sorted insert
// static void insert_buddy(void* addr, uint32_t buddy_size /*Must be 2^n*/)
// {
//     // checks if buddy size is the same as the lsb alone
//     if (buddy_size != (buddy_size & (-buddy_size))) 
//     {
//         debug_print_str("Invalid buddy size");
//         halt();
//     }

//     uint32_t buddy_index = calc_buddy_index_size(buddy_size);

//     buddy_node_t** link = &buddy_blocks[buddy_index].head;
//     buddy_node_t* lst = buddy_blocks[buddy_index].head;
//     while (lst && lst->addr < (uint32_t)addr)
//     {
//         link = &lst->next;
//         lst = lst->next;
//     }

//     buddy_node_t* new_buddy = 
//         (buddy_node_t*)kmalloc(sizeof(buddy_node_t));

//     new_buddy->next = *link;
//     new_buddy->addr = (uint32_t)addr;
//     *link = new_buddy;
// }


// // True if merged, false if not
// static bool merge_iteration(uint32_t size_index, uint32_t max_addr)
// {
//     if (buddy_blocks[size_index].head == NULL) // empty list
//     {
//         return false;
//     }

//     // Set up traversal pointers: prev -> current
//     buddy_node_t** prev_link = &buddy_blocks[size_index].head;
//     buddy_node_t** cur_link = &(buddy_blocks[size_index].head->next);
//     buddy_node_t* cur = *cur_link;

//     max_addr += buddy_blocks[size_index].buddy_size;

//     // Iterate
//     while (cur)
//     {
//         uint32_t prev_addr = (*prev_link)->addr;
//         uint32_t cur_addr = cur->addr;

//         // No merge possible
//         if (cur_addr > max_addr)
//         {
//             return false;
//         }

//         // Can't merge, advance
//         if ((prev_addr ^ cur_addr) != buddy_blocks[size_index].buddy_size)
//         {

//             prev_link = cur_link;
//             cur_link = &cur->next;
//             cur = *cur_link;
//             continue;
//         }

//         // Merge
//         buddy_node_t* prev = *prev_link;
//         *prev_link = cur->next;

//         kfree((uint32_t)prev);
//         kfree((uint32_t)cur);

//         insert_buddy((void*)prev_addr, buddy_blocks[size_index].buddy_size * 2);

//         return true;   
//     }

//     return false;
// }

// static void merge_buddies(uint32_t size_index, uint32_t max_addr)
// {
//     while (size_index <= BUDDY_ORDER_MAX) // Can't merge at BUDDY_ORDER_MAX
//     {
//         if (merge_iteration(size_index, max_addr) == false)
//         {
//             break;
//         }

//         max_addr += buddy_blocks[size_index].buddy_size;

//         ++size_index;
//     }
// }

// void set_phys_pages_struct_type(uint32_t from_virt, uint32_t to_virt /*non inclusive*/, uint32_t struct_addr, uint8_t type)
// {
//     from_virt = from_virt & ~0xFFF;
//     to_virt = to_virt & ~0xFFF;

//     for (uint32_t virt = from_virt & ~0xFFF; virt < to_virt; virt += PAGE_SIZE)
//     {
//         uint32_t phys =(uint32_t)get_phys_addr((void*)virt);
        
//         if (phys == INVALID_PAGE_MEMORY)
//         { 
//             continue;
//         }

//         uint32_t phys_index = phys >> 12;
//         pages[phys_index].type = type;
//         pages[phys_index].struct_addr = (void*)struct_addr;
//     }
// }

// uint32_t reserve_buddy_size(uint32_t size /*Must be 2^n*/, buddy_node_t** req_buddy_node)
// {
//     if (size < PAGE_SIZE)
//     {
//         debug_print_str("Can't ask for size lower than page size");
//         halt();
//     }

//     uint32_t index = calc_buddy_index_size(size) ;

//     // Allocate buddy block of the fitting size
//     if (buddy_blocks[index].head)
//     {
//         buddy_node_t* node = buddy_blocks[index].head;
//         buddy_blocks[index].head = node->next;
        
//         uint32_t addr = node->addr;
//         if (req_buddy_node)
//         {
//             *req_buddy_node = node;
//         }
        
//         return addr;
//     }

//     uint32_t original_index = index;

//     // Find bigger size that can be utilized
//     do
//     {
//         ++index;
//     }
//     while (buddy_blocks[index].head == NULL);


//     // buddy_blocks[index].head != NULL, split by 2 until fitting size
//     while (index > original_index)
//     {
//         // Copy 1 node to lower and allocate another 1
//         buddy_node_t* left_node = buddy_blocks[index].head;
        
//         buddy_blocks[index].head = left_node->next;
    
//         --index;

//         buddy_node_t* right_node = (buddy_node_t*)kmalloc(sizeof(buddy_node_t));
//         right_node->addr = left_node->addr + buddy_blocks[index].buddy_size;
//         right_node->next = NULL;

//         left_node->next = right_node;

//         buddy_blocks[index].head = left_node;
//     }

//     buddy_node_t* node = buddy_blocks[index].head;
//     buddy_blocks[index].head = node->next;
    
//     if (req_buddy_node)
//     {
//         *req_buddy_node = node;
//     }
        
//     return node->addr;
// }

// uint32_t allocate_buddy_size(uint32_t size /*Must be 2^n*/)
// {
//     buddy_node_t* node;

//     uint32_t addr = reserve_buddy_size(size, &node);

//     page_buddy_metadata_t* page_metadata = (page_buddy_metadata_t*)kmalloc(sizeof(page_buddy_metadata_t));

//     page_metadata->buddy_node = node;
//     page_metadata->buddy_size = size;
    
//     set_phys_pages_struct_type(addr, addr+size, (uint32_t)page_metadata, page_type_heap_buddy);

//     return addr;
// }

// void free_buddy_alloc(uint32_t addr)
// {
//     uint32_t phys_page_index = (uint32_t)get_phys_addr((void*)addr)>>12;

//     if (pages[phys_page_index].type != page_type_heap_buddy)
//     {
//         debug_print_str("Must be buddy\n");
//         halt();
//     }
//     page_buddy_metadata_t* buddy_metadata = (page_buddy_metadata_t*)pages[phys_page_index].struct_addr;

//     pages[phys_page_index].type = page_type_heap;
//     pages[phys_page_index].struct_addr = NULL;

//     uint32_t buddy_blocks_index = log2_u32(buddy_metadata->buddy_size) - BUDDY_ORDER_MIN;

//     insert_buddy((void*)addr, buddy_metadata->buddy_size);

//     merge_buddies(buddy_blocks_index, addr);

//     kfree((uint32_t)buddy_metadata);
// }


// void init_early_heap()
// {
//     // Alloc 4mb for early heap
//     alloc_table(get_phys_addr(&early_heap_table), (void*)EARLY_HEAP_VIRT, PAGE_ENTRY_WRITE_KERNEL_FLAGS);
//     early_heap_max_addr = EARLY_HEAP_VIRT + STOR_4MiB;
// }

// void init_heap()
// {
//     // Alloc 4mb for actual heap
//     page_table_t* heap_table = early_alloc_align(sizeof(page_table_t),  PAGE_SIZE);
//     alloc_table(get_phys_addr((void*)heap_table), (void*)HEAP_VIRT, PAGE_ENTRY_WRITE_KERNEL_FLAGS);    

//     set_phys_pages_struct_type(HEAP_VIRT, HEAP_VIRT + STOR_4MiB, (uint32_t)NULL, page_type_heap);

//     init_buddy_alloc_arr();
//     init_slab_alloc_arr();

//     insert_buddy((void*)HEAP_VIRT, STOR_4MiB);
// }

// static void free_phys_pages_from_bitmap(uint8_t bitmap, uint32_t byte_index)
// {
//     for (uint32_t i = 0; i < 8; i++)
//     {
//         if (bitmap & (1 << i))
//         {
//             uint32_t phys_page_index = byte_index * BIT_TO_BYTE + i;
//             pages[phys_page_index].type = page_type_available;
//         }
//     }
// }

// static void setup_phys_pages()
// {
//     uint32_t pages_count = max_memory >> 12;

//     pages = (phys_page_t*)early_alloc_align(pages_count * sizeof(phys_page_t), sizeof(uint32_t));

//     for (uint32_t i = 0; i < pages_count; i++)
//     {
//         pages[i].struct_addr = NULL;
//         pages[i].extra = 0;
//         pages[i].flags = 0;
//         pages[i].type = page_type_kernel_static; // Make all unavailable
//     }

//     // Free all available pages
//     for (uint32_t page_index = 0; page_index < pages_count; page_index++)
//     {
//         free_phys_pages_from_bitmap(pages_allocated[page_index], page_index);
//     }
// }

// void empty_free(uint32_t size){}

// static bool atleast_x_free_slabs(uint32_t index, uint32_t x)
// {
//     uint32_t count = 0;
//     slab_node_t* slab = slab_blocks[index].head;
    
//     while (slab)
//     {
//         count += slab->free_bits;
//         if (count >= x)
//         {
//             return true;
//         }
//         slab = slab->next;
//     }

//     return false;
// }

// static void init_slab_node(slab_node_t* slab_node, uint32_t addr, uint32_t free_amount)
// {
//     slab_node->addr = addr;
//     slab_node->free_bits = free_amount;

//     // Fill ones
//     for (uint32_t i = 0; i < free_amount; i++)
//     {
//         slab_node->bitmap[i / BIT_TO_BYTE] |= (1 << (i % BIT_TO_BYTE));
//     }
//     // Fill zeros
//     for (uint32_t i = free_amount; i < sizeof(slab_node->bitmap) * BIT_TO_BYTE; i++)
//     {
//         slab_node->bitmap[i / BIT_TO_BYTE] &= ~(1 << (i % BIT_TO_BYTE));
//     }
    
// }

// static uint32_t allocate_first_free_slab(uint32_t size_index, slab_node_t** used_slab)
// {
//     slab_node_t** link = &slab_blocks[size_index].head;
//     slab_node_t* node = slab_blocks[size_index].head;

//     bool found = false;
//     while(node)
//     {
//         if (node->free_bits)
//         {
//             found = true;
//             break;
//         }

//         link = &node->next;
//         node = node->next;
//     }

//     if (!found)
//     {
//         return INVALID_MEMORY;
//     }

//     uint32_t max_free_amount = PAGE_SIZE / slab_blocks[size_index].slab_size;

//     uint32_t bit;
//     for (bit = 0; bit < max_free_amount; bit++)
//     {
//         if (node->bitmap[bit / BIT_TO_BYTE] & (1 << (bit % BIT_TO_BYTE)))
//         {
//             break;
//         }
//     }

//     if (bit == max_free_amount)
//     {    
//         debug_print_str("It said there are free bits, there werent");
//         halt();
//     }

//     node->bitmap[bit / BIT_TO_BYTE] &= ~(1 << (bit % BIT_TO_BYTE));
//     node->free_bits--;

//     if (used_slab)
//     {
//         *used_slab = node;
//     }

//     if (node->free_bits == 0)
//     {
//         *link = node->next;
//     }
    
//     return node->addr + bit * slab_blocks[size_index].slab_size;
// }

// // Ensures there are enough slab nodes and slab metadata that can be created
// // for at one object.
// static void reserve_slab_objects()
// {
//     /*
//     Can make 2 pages for both the slab_node_t and page_slab_metadata_t
//     If only slab_node_t has no way to allocate then only that
//     same with page_slab_metadata_t
//     */
//     const uint32_t slab_metadata_size = align_pow2(sizeof(page_slab_metadata_t));
//     const uint32_t slab_metadata_size_index = calc_slab_index_size( slab_metadata_size );
//     const uint32_t slab_node_size = align_pow2(sizeof(slab_node_t));
//     const uint32_t slab_node_size_index =  calc_slab_index_size(slab_node_size);

//     // page_slab_metadata_size if we have less than or equal 3, allocate
//     // slab_node_size if we have less than or equal 3, allocate
//     bool will_alloc_slab_metadata = atleast_x_free_slabs(slab_metadata_size_index, 3) == false;
//     bool will_alloc_slab_node     = atleast_x_free_slabs(slab_node_size_index, 3) == false;


//     if (will_alloc_slab_metadata && will_alloc_slab_node)
//     {
//         uint32_t slab_metadata_addr = reserve_buddy_size(PAGE_SIZE, NULL);
//         uint32_t slab_node_addr = reserve_buddy_size(PAGE_SIZE, NULL);

//         uint32_t slab_metadata_page_index = ((uint32_t)get_phys_addr((void*)slab_metadata_addr) >> 12);
//         uint32_t slab_node_page_index = ((uint32_t)get_phys_addr((void*)slab_node_addr) >> 12);

//         // Allocate 2 page metadata and 2 slab node
//         page_slab_metadata_t* slab_metadata_alloc_node = (page_slab_metadata_t*)slab_metadata_addr;
//         page_slab_metadata_t* slab_metadata_alloc_metadata = (page_slab_metadata_t*)(slab_metadata_addr + slab_metadata_size);

//         slab_node_t* slab_node_alloc_node = (slab_node_t*)slab_node_addr;
//         slab_node_t* slab_node_alloc_metadata = (slab_node_t*)(slab_node_addr + slab_node_size);

//         // Handle alloc_metadata first
//         slab_metadata_alloc_metadata->slab_node = slab_node_alloc_metadata;
//         slab_metadata_alloc_metadata->slab_size = slab_metadata_size;
        
//         init_slab_node(slab_node_alloc_metadata, slab_metadata_addr, PAGE_SIZE / slab_metadata_size);

//         // Free 2 first bits in each node, used by this logic
//         slab_node_alloc_metadata->free_bits -= 2;
//         slab_node_alloc_metadata->bitmap[0] &= ~0b11;

//         pages[slab_metadata_page_index].extra = 0;
//         pages[slab_metadata_page_index].flags = 0;
//         pages[slab_metadata_page_index].type = page_type_heap_slab;
//         pages[slab_metadata_page_index].struct_addr = &slab_metadata_alloc_metadata;


//         // Handle alloc_node second
//         slab_metadata_alloc_node->slab_node = slab_node_alloc_node;
//         slab_metadata_alloc_node->slab_size = slab_node_size;
        
//         init_slab_node(slab_node_alloc_node, slab_node_addr, PAGE_SIZE / slab_node_size);
        

//         // Free 2 first bits in each node, used by this logic
//         slab_node_alloc_node->free_bits -= 2;
//         slab_node_alloc_node->bitmap[0] &= ~0b11;

//         pages[slab_node_page_index].extra = 0;
//         pages[slab_node_page_index].flags = 0;
//         pages[slab_node_page_index].type = page_type_heap_slab;
//         pages[slab_node_page_index].struct_addr = &slab_metadata_alloc_node;

//         // Add both nodes to the lists:
//         slab_node_alloc_metadata->next = slab_blocks[slab_metadata_size_index].head ? slab_blocks[slab_metadata_size_index].head->next : NULL;
//         slab_blocks[slab_metadata_size_index].head = slab_node_alloc_metadata;

//         slab_node_alloc_node->next = slab_blocks[slab_node_size_index].head ? slab_blocks[slab_node_size_index].head->next : NULL;
//         slab_blocks[slab_node_size_index].head = slab_node_alloc_node;        

//         return;
//     }
//     if (will_alloc_slab_metadata)
//     {
//         uint32_t slab_metadata_page_addr = reserve_buddy_size(PAGE_SIZE, NULL);
//         uint32_t slab_metadata_index = ((uint32_t)get_phys_addr((void*)slab_metadata_page_addr) >> 12);

//         // Allocate page metadata (using the new page) and slab node (using normal)
//         uint32_t slab_node_addr = allocate_first_free_slab(slab_node_size_index, NULL);
//         slab_node_t* slab_node = (slab_node_t*)(slab_node_addr);
//         init_slab_node(slab_node, slab_metadata_page_addr, PAGE_SIZE / slab_node_size);

//         // Remove one use of the metadata 
//         slab_node->free_bits--;
//         slab_node->bitmap[0] &= ~0b1;

//         page_slab_metadata_t* slab_metadata = (page_slab_metadata_t*)slab_metadata_page_addr;
//         slab_metadata->slab_node = slab_node;
//         slab_metadata->slab_size = slab_metadata_size;

//         pages[slab_metadata_index].extra = 0;
//         pages[slab_metadata_index].flags = 0;
//         pages[slab_metadata_index].type = page_type_heap_slab;
//         pages[slab_metadata_index].struct_addr = &slab_metadata;

//         return;
//     }
//     if (will_alloc_slab_node)
//     {
//         uint32_t slab_node_page_addr = reserve_buddy_size(PAGE_SIZE, NULL);
//         uint32_t slab_node_page_index = ((uint32_t)get_phys_addr((void*)slab_node_page_addr) >> 12);

//         // Allocate page metadata (using normal) and slab node (using the new page)
//         slab_node_t* slab_node = (slab_node_t*)slab_node_page_addr;
//         init_slab_node(slab_node, slab_node_page_addr, PAGE_SIZE / slab_node_size);
//         // Remove one use of the metadata 
//         slab_node->free_bits--;
//         slab_node->bitmap[0] &= ~0b1;

//         uint32_t slab_metadata_addr = allocate_first_free_slab(slab_metadata_size_index, NULL);
//         page_slab_metadata_t* slab_metadata = (page_slab_metadata_t*)(slab_metadata_addr);
        
//         slab_metadata->slab_node = slab_node;
//         slab_metadata->slab_size = slab_metadata_size;

//         pages[slab_node_page_index].extra = 0;
//         pages[slab_node_page_index].flags = 0;
//         pages[slab_node_page_index].type = page_type_heap_slab;
//         pages[slab_node_page_index].struct_addr = &slab_metadata;

//         return;
//     }
// }

// static uint32_t allocate_slab_size(uint32_t obj_size)
// {
//     if (obj_size < SLAB_MIN_SIZE)
//     {
//         debug_print_str("slab size was not correct");
//         halt();
//     }

//     uint32_t obj_slab_index = calc_slab_index_size(obj_size); 

//     reserve_slab_objects();
    
//     if (slab_blocks[obj_slab_index].head == NULL)
//     {
//         // Cant allocate a slab_node_t or page slab metadata for the allocation
//         uint32_t slabs_size_index = calc_slab_index_size( sizeof(slab_node_t) );
//         uint32_t page_metadata_size_index = calc_slab_index_size(sizeof(page_slab_metadata_t));

//         // MAYBE ADD THIS LATER
//         // MAYBE ADD THIS LATER
//         // MAYBE ADD THIS LATER  (check for whether the object has the same index as the page && slab metadata)
//         // if (slabs_size_index != obj_slab_index && page_metadata_size_index != obj_slab_index)
        
//         uint32_t buddy_addr =  reserve_buddy_size(PAGE_SIZE, NULL);

//         slab_node_t* slab_node = kmalloc(sizeof(slab_node_t));
//         init_slab_node(slab_node, buddy_addr, PAGE_SIZE / obj_size);

//         slab_blocks[obj_slab_index].head = slab_node;
//         slab_blocks[obj_slab_index].slab_size = obj_size;

//         page_slab_metadata_t* slab_metadata = kmalloc(sizeof(page_slab_metadata_t));
//         slab_metadata->slab_node = slab_node;
//         slab_metadata->slab_size = obj_size;

//         uint32_t page_index = (uint32_t)get_phys_addr((void*)buddy_addr) >> 12;
//         pages[page_index].struct_addr = slab_metadata;
//         pages[page_index].type = page_type_heap_slab;
//         pages[page_index].flags = 0;
//         pages[page_index].extra = 0;

//         // Another reserve because 2 objects were used already
//         reserve_slab_objects();
//     }

//     slab_node_t* slab_node;
//     uint32_t obj_addr = allocate_first_free_slab(obj_slab_index, &slab_node);
    
//     uint32_t page_index = (uint32_t)get_phys_addr((void*)obj_addr) >> 12;

//     page_slab_metadata_t* slab_metadata = kmalloc(sizeof(page_slab_metadata_t));
//     slab_metadata->slab_node = slab_node;
//     slab_metadata->slab_size = obj_size;

//     pages[page_index].struct_addr = slab_metadata;
//     pages[page_index].type = page_type_heap_slab;
//     pages[page_index].flags = 0;
//     pages[page_index].extra = 0;
    
//     return obj_addr;
// }

// static void free_slab_alloc(uint32_t obj_addr)
// {
//     uint32_t phys_page_index = (uint32_t)get_phys_addr((void*)obj_addr)>>12;

//     page_slab_metadata_t* metadata = (page_slab_metadata_t*) pages[phys_page_index].struct_addr;

//     slab_node_t* node = metadata->slab_node;
    
//     uint32_t size_index = calc_slab_index_size(metadata->slab_size); 

//     // Reinsert if needed
//     if (node->free_bits == 0)
//     {
//         slab_node_t** link_it = &slab_blocks[size_index].head;
//         slab_node_t* node_it = slab_blocks[size_index].head;

//         while (node_it && node_it->addr < obj_addr)
//         {
//             link_it = &node_it->next;
//             node_it = node_it->next;
//         }

//         (*link_it)->next = node;
//         node->next = node_it;
//     }
    
//     uint32_t bit = (obj_addr - node->addr) / (metadata->slab_size); 
//     if (node->bitmap[bit / BIT_TO_BYTE] & (1 << (bit % BIT_TO_BYTE)))
//     {
//         debug_print_str("Bit for slab is already freed");
//         halt();
//     }

//     node->free_bits++;

//     node->bitmap[bit / BIT_TO_BYTE] |= 1 << (bit % BIT_TO_BYTE);
// }

// void* alloc(uint32_t size)
// {
//     if (size == 0)
//     {
//         debug_print_str("Invalid `alloc` size");
//         halt();
//     }
//     size = align_pow2(size);

//     if (size < PAGE_SIZE)
//     {
//         return (void*)allocate_slab_size(size);
//     }
//     else
//     {
//         return (void*)allocate_buddy_size(size);
//     }
// }

// void free(void* addr)
// {
//     uint32_t phys_page_index = (uint32_t)get_phys_addr(addr) >> 12;
//     if (pages[phys_page_index].type == page_type_heap_buddy)
//     {
//         free_buddy_alloc((uint32_t)addr);
//     }
//     else if (pages[phys_page_index].type == page_type_heap_slab)
//     {
//         free_slab_alloc((uint32_t)addr);
//     }
//     else
//     {
//         debug_print_str("Invalid address to free!");
//     }
// }

// page_buddy_metadata_t* virt_addr_to_buddy_struct(uint32_t addr)
// {
//     return (page_buddy_metadata_t*)pages[(uint32_t)get_phys_addr((void*)addr) >> 12].struct_addr;
// }
// page_slab_metadata_t* virt_addr_to_slab_struct(uint32_t addr)
// {
//     return (page_slab_metadata_t*)pages[(uint32_t)get_phys_addr((void*)addr) >> 12].struct_addr;
// }


// static void debug_buddy_blocks()
// {
//     debug_print_str("4KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_4KiB)].head ));
//     debug_print_str(" | ");
//     debug_print_str("8KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_8Kib)].head ));
//     debug_print_str(" | ");
//     debug_print_str("16KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_16Kib)].head));
//     debug_print_str(" | ");
//     debug_print_str("32KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_32Kib)].head ));
//     debug_print_str("\n");
//     debug_print_str("64KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_64Kib)].head ));
//     debug_print_str(" | ");
//     debug_print_str("128KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_128Kib)].head ));
//     debug_print_str(" | ");
//     debug_print_str("256KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_256Kib)].head ));
//     debug_print_str(" | ");
//     debug_print_str("512KiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_512Kib)].head ));
//     debug_print_str("\n");
//     debug_print_str("1MiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_1MiB)].head ));
//     debug_print_str(" | ");
//     debug_print_str("2MiB:");
//     debug_print_int_nonewline(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_2MiB)].head ));
//     debug_print_str(" | ");
//     debug_print_str("4MiB:");
//     debug_print_int(buddy_block_length(buddy_blocks[calc_buddy_index_size(STOR_4MiB)].head ));
// }

// void run_heap_stress_test()
// {
//     debug_buddy_blocks(); // snapshot before frees
//     // Slab allocations
//     void* a1 = alloc(32);    debug_print((uint32_t)a1);
//     void* a2 = alloc(60);    debug_print((uint32_t)a2);
//     void* a3 = alloc(128);   debug_print((uint32_t)a3);
//     void* a4 = alloc(256);   debug_print((uint32_t)a4);

//     // Slab overflow → goes to buddy
//     void* a5 = alloc(300);   debug_print((uint32_t)a5);

//     // Buddy allocations
//     void* b1 = alloc(4096);      debug_print((uint32_t)b1);
//     void* b2 = alloc(8192);      debug_print((uint32_t)b2);
//     void* b3 = alloc(16384);     debug_print((uint32_t)b3);
//     void* b4 = alloc(32768);     debug_print((uint32_t)b4);
//     void* b5 = alloc(1 << 20);   debug_print((uint32_t)b5); // 1MiB

//     // Misaligned size → buddy waste
//     void* b6 = alloc(4097);      debug_print((uint32_t)b6);

//     // Mixed slab
//     void* d = alloc(128);        debug_print((uint32_t)d);

//     // Slab free order stress test
//     void* p1 = alloc(64);        debug_print((uint32_t)p1);
//     void* p2 = alloc(64);        debug_print((uint32_t)p2);
//     void* p3 = alloc(64);        debug_print((uint32_t)p3);

//     debug_buddy_blocks(); // snapshot before frees

//     // Free in non-linear order
//     free(p2);
//     free(p1);
//     free(p3);

//     free(d);

//     // Free everything else
//     free(a1); free(a2); free(a3); free(a4); free(a5);
//     free(b1); free(b2); free(b3); free(b4); free(b5); free(b6);

//     debug_buddy_blocks(); // snapshot after frees
// }

// void setup_heap()
// {
//     pages = NULL;

//     init_early_heap();

//     kmalloc = early_alloc;
//     kfree = empty_free;

//     setup_phys_pages();

//     init_heap();

//     run_heap_stress_test();

//     halt();
// }