[BITS 32]

; The responsiblity is on the irq and isr_wrappers to decide which IRQs are created
; not all will be create and thus that's a responsibility to add another irq if 
; needed, those are hardcoded. Here only 32 predefined by intel, and 16 for PIC IRQs
;

extern idt_c_handler
%macro IRQ 1
section .text
global isr%1
isr%1:
    push dword 0         ; user ss
    push dword 0         ; user esp
    pushad               ; push general purpose registers
    push dword %1        ; irq_index
    push dword 0         ; fake err_code
    push esp             ; pointer to irq_frame_t
    call idt_c_handler
    add esp, 12          ; pop arg, fake err_code and irq_index
    popad                ; restore registers
    add esp, 8           ; pop both user ss & esp
    iretd
%endmacro

%macro ISR_NO_ERR 1
section .text
global isr%1
isr%1:
    push dword 0         ; user ss
    push dword 0         ; user esp
    pushad               ; push general purpose registers
    push dword %1        ; irq_index
    push dword 0         ; fake err_code
    push esp             ; pointer to irq_frame_t
    call idt_c_handler
    add esp, 12          ; pop arg, fake err_code and irq_index
    popad                ; restore registers
    add esp, 8           ; pop both user ss & esp
    iretd
%endmacro

%macro ISR_ERR 1
section .text
global isr%1
isr%1:
    push dword 0         ; user esp
    push dword 0         ; user ss
    pushad               ; push general purpose registers
    mov eax, [esp + 40]  ; fetch CPU err_code, pushad (32) + 2 dwords for user
    push dword %1        ; irq_index
    push eax             ; err_code
    push esp             ; pointer to irq_frame_t
    call idt_c_handler
    add esp, 12          ; pop arg, fake err_code and irq_index
    popad                ; restore registers
    add esp, 12          ; pop user_esp & user_ss + error
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

IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47