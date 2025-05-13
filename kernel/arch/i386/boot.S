# Multiboot Header
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Page Tables and Stack
.section .data
.align 4096
page_directory:
.skip 0x00001000  # 4 KiB (Page Directory)

page_kernel_tables:
.skip 0x00002000  # 8 KiB (2 MiB) 
page_boot_tables:
.skip 0x00001000  # 4 KiB (1 MiB)



.section .rodata
.align 8
gdt:
    .skip 40 # 5 - 8 byte entries
gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt
gdt_end:



.section .bss
.align 16
stack_bottom:
.skip 0x004000 # 16 KiB
stack_top:

# Entry Point
.section .text
.global _start
.type _start, @function
_start:
    # Set up the stack
    movl $stack_top, %esp
    andl $0xFFFFFFF0, %esp

    # Preserve the multiboot info pointer in %ebx
    pushl %ebx

    # Call global constructors
    call _init

init_gdt:
    # Initialize gdt
    pushl $gdt
    call setup_gdt
    addl $4, %esp
    # Load the new GDT
    lgdt gdt_descriptor          

    ljmp $0x08, $reload_CS

reload_CS:
   # Reload data segment registers:
   # 0x10 = kernel data segment
   movw  $0x10, %ax 
   movw  %ax, %ds
   movw  %ax, %es
   movw  %ax, %fs
   movw  %ax, %gs
   movw  %ax, %ss

init_paging:
    # Initialize paging
    pushl $page_boot_tables
    pushl $page_kernel_tables
    pushl $page_directory
    call setup_paging
    addl $12, %esp

    # Set the page directory
    movl $page_directory, %eax
    movl %eax, %cr3
    
    # Enable paging
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    
    # Transfer control to the main kernel (high kernel address)
    movl $kernel_main, %eax
    addl $0xBFF00000, %eax # 0xC0000000 - 0x0010000
    call *%eax

    # Hang if kernel_main returns
    cli
1:  hlt
    jmp 1b
.size _start, . - _start
