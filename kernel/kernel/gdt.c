#include <stdint.h>

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)
 
#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed
 
#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR
 
#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR

#define DESCRIPTORS_AMOUNT 5

typedef struct gdt_entry 
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_description
{
    uint16_t limit;
    uint32_t offset;
} __attribute__((packed)) gdt_description_t ;

gdt_entry_t gdt_entries[DESCRIPTORS_AMOUNT]  __attribute__((aligned(16)));

static void set_gdt_entry(gdt_entry_t* entry, uint32_t base, uint32_t limit, uint16_t flag)
{
    entry->limit_low    = limit & 0xFFFF;              // set limit low bits 15:00
    entry->granularity  = (limit >> 16) & 0x0F;        // set limit high bits 19:16

    entry->base_low     = base & 0xFFFF;               // set base bits 15:00
    entry->base_middle  = (base >> 16) & 0xFF;         // set base bits 23:16
    entry->base_high    = (base >> 24) & 0xFF;         // set base bits 31:24

    entry->access       = flag & 0xFF;                 // set access bits 07:00
    entry->granularity |= (flag >> 8) & 0xF0;          // set flags bits 03:00
}


static inline void load_gdt(gdt_description_t* gdtr)
{
    // Load the GDT and reload the segment registers
    asm volatile (
        "lgdt (%0)\n\t"       // Load the GDT
        "ljmp $0x08, $reload_cs\n\t"  // Long jump to flush the instruction pipeline
        "reload_cs:\n\t"
        "movw $0x10, %%ax\n\t" // Load the data segment selectors
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        :
        : "r" (gdtr)
        : "memory", "eax"
    );
}


void setup_gdt()
{
    // Setup GDT entries

    // First GDT entry to null
    set_gdt_entry(&gdt_entries[0], 0, 0,0);

    set_gdt_entry(&gdt_entries[1], 0, 0xFFFFF, GDT_CODE_PL0);
    set_gdt_entry(&gdt_entries[2], 0, 0xFFFFF, GDT_DATA_PL0);
    set_gdt_entry(&gdt_entries[3], 0, 0xFFFFF, GDT_CODE_PL3);
    set_gdt_entry(&gdt_entries[4], 0, 0xFFFFF, GDT_DATA_PL3);

    // Setup the GDTR
    gdt_description_t gdtr;
    gdtr.offset = (uint32_t)gdt_entries; 
    gdtr.limit = sizeof(gdt_entries) - 1; 

    // Load the GDT to the CPU
    load_gdt((uint32_t)&gdtr);
}
