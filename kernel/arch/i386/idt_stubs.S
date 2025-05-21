.macro ISR_NO_ERR n
.global isr\n
isr\n:
    pusha
    pushl $\n            # Interrupt number
    pushl $0             # Fake error code
    call idt_c_handler
    addl $8, %esp
    popa
    iret
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
    pusha
    pushl 32(%esp)       # Error code (right after registers 4 * 8 = 32)
    pushl $\n            # Interrupt number
    call idt_c_handler
    addl $8, %esp
    popa
    addl $4, %esp        # Remove error code
    iret
.endm





ISR_NO_ERR 0
ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR 8
ISR_NO_ERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_ERR 17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_ERR 21
ISR_NO_ERR 22 # RESERVED
ISR_NO_ERR 23 # RESERVED
ISR_NO_ERR 24 # RESERVED
ISR_NO_ERR 25 # RESERVED
ISR_NO_ERR 26 # RESERVED
ISR_NO_ERR 27 # RESERVED
ISR_NO_ERR 28
ISR_ERR 29
ISR_ERR 30
ISR_NO_ERR 31
