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

section .bss.boot
extern kernel_stack_bottom        ; physical stack bottom
extern __va_pa_off                ; virtual = phys + off


; Entry Point
section .text.boot

extern entry_main

global _start
jmp _start
_start:
    cli
    cld

    ; Save GRUB data
    mov esi, eax
    mov edi, ebx

    ; Set up the stack
    ; eax = physical bottom
    mov eax, kernel_stack_bottom
    sub eax, __va_pa_off 

    mov esp, eax
    mov ebp, eax

    ; call entry_main(uint32_t magic, multiboot_info_t* mbd)
    push edi
    push esi
    call entry_main

    ; Hang if kernel_main returns
    cli
.hang:  
    hlt
    jmp .hang
