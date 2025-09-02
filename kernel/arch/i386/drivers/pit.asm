BITS 32

section .bss
; Fractions of 1 ms since timer
global system_timer_fractions
system_timer_fractions:  dd 0
; Number of whole ms since timer initialized
global system_timer_ms
system_timer_ms:         dd 0
; Fractions of 1 ms between IRQs
IRQ0_fractions:          dd 0
; Number of whole ms between IRQs
IRQ0_ms:                 dd 0
; Actual frequency of PIT
IRQ0_frequency:          dd 0
; Current PIT reload value
PIT_reload_value:        dw 0

section .text
extern pit_callback
global IRQ0_handler
IRQ0_handler:
    push eax
    push ebx

    mov eax, [IRQ0_fractions]
    mov ebx, [IRQ0_ms]                  ; EAX:EBX = amount of time between IRQs

    add  [system_timer_fractions], eax  ; Update system timer tick fractions
    adc  [system_timer_ms], ebx         ; Update system timer tick milliseconds

    ; Send the EOI to the PIC (master)
    mov al, 0x20
    out 0x20, al

    ; Call pit callback (pit_callback is pointer to function)
    mov eax, [pit_callback]
    call eax

    pop ebx
    pop eax
    
    iretd

; Input:
;   EBX = Desired PIT frequency in Hz
global setup_pit_asm
setup_pit_asm:
    push eax
    push ebx
    push edx

; Use EBX to calculate the rounded frequency
calc_rounded_frequency:
    mov eax, 0x10000                ; EAX = reload value for slowest
    cmp ebx, 18                     ; Is the frequency too low?
    jbe get_reload_value            ; Yes → use slowest reload value

    mov eax, 1                      ; EAX = reload value for fastest
    cmp ebx, 1193181                ; Is the frequency too high?
    jae get_reload_value            ; Yes → use fastest reload value

    ; Calculate reload_value ≈ (3579545 / 3) / desired_freq
    mov eax, 3579545
    xor edx, edx
    div ebx                          ; EAX = 3579545 / desired_freq, EDX = remainder

    ; Round to nearest before dividing by 3
    cmp edx, 1789772                 ; 3579545 / 2
    jbe rounded_freq_div_3
    inc eax

rounded_freq_div_3:
    mov ebx, 3
    xor edx, edx
    div ebx                          ; EAX = (3579545 / desired_freq) / 3

    ; Round if remainder >= 1 (3/2 threshold for integer division by 3 here)
    cmp edx, 1
    jb  get_reload_value
    inc eax

; Store the reload value and calculate the actual frequency
get_reload_value:
    push eax                         ; Save reload value on stack
    mov [PIT_reload_value], ax       ; Save 16-bit reload value
    mov ebx, eax                     ; EBX = reload value

    ; Compute corrected frequency:
    ; frequency ≈ (3579545 / 3) / reload_value with rounding
    mov eax, 3579545
    xor edx, edx
    div ebx                          ; EAX = 3579545 / reload_value, EDX = remainder
    cmp edx, 1789772                 ; Round to nearest
    jb  rounded_hz_div_3
    inc eax

rounded_hz_div_3:
    mov ebx, 3
    xor edx, edx
    div ebx                          ; EAX = (3579545 / reload_value) / 3
    cmp edx, 1
    jb  calc_amount_time
    inc eax

; Calculate the amount of time between IRQs in 32.32 fixed point
; time_ms = reload_value * 3000 / 1193181 ≈ reload_value * 3000 * 2^42 / 3579545 / 2^10
calc_amount_time:
    mov [IRQ0_frequency], eax        ; Store actual frequency for later display

    pop ebx                          ; EBX = reload value
    mov eax, 0xDBB3A062              ; EAX = floor(3000 * 2^42 / 3579545)
    mul ebx                          ; EDX:EAX = reload_value * const

    ; Divide by 2^10 → right shift 10 over 64-bit in EDX:EAX
    shrd eax, edx, 10
    shr  edx, 10

    ; Store 32.32 fixed-point components
    mov [IRQ0_ms], edx
    mov [IRQ0_fractions], eax

; Program the PIT channel 0 in mode 2 (rate generator), lobyte/hibyte
program_pit_channel:
    pushf
    cli

    mov al, 00110100b                ; ch0, lobyte/hibyte, mode 2, binary
    out 0x43, al

    mov ax, [PIT_reload_value]
    out 0x40, al                     ; Low byte
    mov al, ah
    out 0x40, al                     ; High byte

    popf

    pop eax
    pop ebx
    pop edx
    ret
