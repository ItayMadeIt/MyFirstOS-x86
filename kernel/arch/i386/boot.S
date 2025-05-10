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
    # Preserve the multiboot info pointer in %ebx
    pushl %ebx

    # Set up the stack
    movl $stack_top, %esp
    andl $0xFFFFFFF0, %esp


    movl %esp, %eax
    pushl %eax
    # call debug_print
    addl $4, %esp

    # Call global constructors
    call _init

    # Initialize paging
    pushl $page_boot_tables
    pushl $page_kernel_tables
    pushl $page_directory
    call setup_paging
    addl $12, %esp

    # Set the page directory
    movl $page_directory, %eax
    movl %eax, %cr3

    movl $0xC000000, %ebx
    addl $kernel_main, %ebx
    subl $0x0010000, %ebx


    
    # Enable paging
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    
    movb $1, 0xC0000000

    # pushl $page_directory
    # call dump_directory
    # addl $4, %esp

    # Transfer control to the main kernel (high address)
    call kernel_main

    # Hang if kernel_main returns
    cli
1:  hlt
    jmp 1b
.size _start, . - _start
