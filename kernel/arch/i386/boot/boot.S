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


.section .bss
.align 16
stack_bottom:
.skip 0x010000 # 64 KiB
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
    pushl %eax

    # Call global constructors
    call _init

    call entry_main
    addl $8, %esp

    # Hang if kernel_main returns
    cli
1:  hlt
    jmp 1b
.size _start, . - _start
