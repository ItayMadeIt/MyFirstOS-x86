[BITS 32]


section .bss
IRQ1_scancode: resd 0

section .text
extern key_callback
global IRQ1_handler
IRQ1_handler:
    pushad

    call key_callback

    popad
    iretd