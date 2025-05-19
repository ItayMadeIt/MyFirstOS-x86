# Multiboot Header
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.set IDT_ENTRIES, 256
.set IDT_ENTRY_SIZE, 8 # Size of an IDT entry, called a gate descriptor

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM


.section .rodata
.align 8
gdt:
    .skip 40 # 5 - 8 byte entries
gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt
gdt_end:

idt_table:
    .skip IDT_ENTRIES * IDT_ENTRY_SIZE
idt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt

# Page Tables and Stack
.section .data
.align 4096
page_directory:
.skip 0x00001000  # 4 KiB (Page Directory)

page_kernel_tables:
.skip 0x00002000  # 8 KiB (2 MiB) 
page_boot_tables:
.skip 0x00001000  # 4 KiB (1 MiB)


.section .bss
.align 16
stack_bottom:
.skip 0x004000 # 16 KiB
stack_top:

# Entry Point
.section .text
.global _start
.type _start, @function
jmp _start


_start:
    # Set up the stack
    movl $stack_top, %esp
    andl $0xFFFFFFF0, %esp

    # Preserve the multiboot info pointer in %ebx
    pushl %ebx

    # Call global constructors
    call _init

    call entry_main

    # Hang if kernel_main returns
    cli
1:  hlt
    jmp 1b
.size _start, . - _start
