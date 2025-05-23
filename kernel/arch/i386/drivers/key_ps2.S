.section .bss

IRQ1_scancode:         .long 0

.section .text
.global IRQ1_handler
IRQ1_handler:
    pusha

    call key_callback

    popa
    iret