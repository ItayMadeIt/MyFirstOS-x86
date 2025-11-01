#include <core/defs.h>
#include <early/defs.h>
#include <arch/i386/core/gdt.h>

EARLY_BSS_SECTION
gdt_entry_t gdt_entries[DESCRIPTORS_AMOUNT]  __attribute__((aligned(16)));

EARLY_TEXT_SECTION
static void set_gdt_entry(gdt_entry_t* entry, u32 base, u32 limit, u16 flag)
{
    entry->limit_low    = limit & 0xFFFF;              // set limit low bits 15:00
    entry->granularity  = (limit >> 16) & 0x0F;        // set limit high bits 19:16

    entry->base_low     = base & 0xFFFF;               // set base bits 15:00
    entry->base_middle  = (base >> 16) & 0xFF;         // set base bits 23:16
    entry->base_high    = (base >> 24) & 0xFF;         // set base bits 31:24

    entry->access       = flag & 0xFF;                 // set access bits 07:00
    entry->granularity |= (flag >> 8) & 0xF0;          // set flags bits 03:00
}

EARLY_TEXT_SECTION
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

EARLY_TEXT_SECTION
void init_gdt()
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
    gdtr.offset = (u32)gdt_entries; 
    gdtr.limit = sizeof(gdt_entries) - 1; 

    // Load the GDT to the CPU
    load_gdt(&gdtr);
}
