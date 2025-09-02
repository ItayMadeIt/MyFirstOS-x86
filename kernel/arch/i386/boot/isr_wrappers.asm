[BITS 32]

extern idt_c_handler

%macro ISR_NO_ERR 1
section .text
global isr%1
isr%1:
    pushad
    push dword %1        ; Interrupt number
    push dword 0         ; Fake error code
    call idt_c_handler
    add esp, 8
    popad
    iretd
%endmacro

%macro ISR_ERR 1
section .text
global isr%1
isr%1:
    pushad
    push dword [esp + 32] ; Error code (after pushad)
    push dword %1         ; Interrupt number
    call idt_c_handler
    add esp, 8
    popad
    add esp, 4
    iretd
%endmacro

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
ISR_NO_ERR 22 ; RESERVED
ISR_NO_ERR 23 ; RESERVED
ISR_NO_ERR 24 ; RESERVED
ISR_NO_ERR 25 ; RESERVED
ISR_NO_ERR 26 ; RESERVED
ISR_NO_ERR 27 ; RESERVED
ISR_NO_ERR 28
ISR_ERR 29
ISR_ERR 30
ISR_NO_ERR 31
