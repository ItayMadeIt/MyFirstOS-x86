[BITS 32]


section .bss
IRQ1_scancode: resd 0

section .text
extern ps2_key_callback
global ps2_handler
ps2_handler:
    pushad
    pushf

    call ps2_key_callback

    popf
    popad
    iretd