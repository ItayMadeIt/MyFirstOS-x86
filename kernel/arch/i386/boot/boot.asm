[BITS 32]

; Multiboot Header
%define MB_ALIGN    (1<<0)
%define MB_MEMINFO  (1<<1)
%define MB_FLAGS    0x3 ; MB_ALIGN | MB_MEMINFO
%define MB_MAGIC    0x1BADB002
%define MB_CHECKSUM (-(MB_FLAGS + MB_MAGIC)) 

section .multiboot
align 4
    dd MB_MAGIC              ; magic
    dd MB_FLAGS              ; flags (ALIGN | MEMINFO)
    dd MB_CHECKSUM   ; checksum

section .bss
align 16
stack_bottom:
    resb 0x10000    ; 64 KiB
stack_top:


; Entry Point
section .text
extern entry_main
global _start
jmp _start
_start:
    ; Set up the stack
    mov esp, stack_top
    and esp, 0xFFFFFFF0

    ; Preserve the multiboot info pointer in `ebx`
    push ebx
    push eax
    
    call entry_main
    add esp, 8

    ; Hang if kernel_main returns
    cli
.hang:  
    hlt
    jmp .hang
