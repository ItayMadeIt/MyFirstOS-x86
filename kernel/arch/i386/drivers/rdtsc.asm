section .bss
IRQ0_fractions:         dd 0
IRQ0_ms:                dd 0

global setup_rdtsc
setup_rdtsc:
    mov eax, 1
    CPUID
    mov eax, 0x80000007
    CPUID
    
    
