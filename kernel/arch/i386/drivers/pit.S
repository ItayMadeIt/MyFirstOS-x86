.section .bss

system_timer_fractions: .long 0 # Fractions of 1 ms since timer 
system_timer_ms:        .long 0 # Number of whole ms since timer initialized
IRQ0_fractions:         .long 0 # Fractions of 1 ms between IRQs
IRQ0_ms:                .long 0 # Number of whole ms between IRQs
IRQ0_frequency:         .long 0  # Actual frequency of PIT
PIT_reload_value:       .word 0 # Current PIT reload value

.section .text
.global IRQ0_handler
IRQ0_handler:
    pushl %eax
	pushl %ebx

	movl IRQ0_fractions, %eax
	movl IRQ0_ms, %ebx                    # eax:ebx = amount of time between IRQs

	addl %eax, system_timer_fractions     # Update system timer tick fractions
	adcl %ebx, system_timer_ms            # Update system timer tick milli-seconds

    pushl system_timer_ms
    call debug_print_int
    addl $4, %esp

    # Send the EOI to the PIC
	movb $0x20, %al
    outb %al, $0x20  

	popl %ebx
	popl %eax
	iret


 # Input
 # ebx   Desired PIT frequency in Hz
.global setup_pit_asm
setup_pit_asm:
    pushl %eax
    pushl %ebx
    pushl %edx
    
# Use EBX to calculate the rounded frequency
calc_rounded_frequency: 
    movl $0x10000, %eax             # EAX = reload value for slowest
    cmpl $18, %ebx                  # Is the frequency too low
    jbe get_reload_value            # Yes, use slowest reload value

    movl $1, %eax                   # EAX = reload value for fastest
    cmpl $1193181, %ebx             # Is the frequency too high
    jae get_reload_value            # Yes, use fastest reload value

    # Calculate reload_value = 3579545 / desired_freq(ebx) 
    # (3579545 / 3 = ~1193182(value of max_value) better accuracy)
    movl $3579545, %eax
    xorl %edx, %edx
    divl %ebx

    # EDX = remainder 
    cmpl $1789772, %edx  # EDX <= 1789772
    jbe rounded_freq_div_3
    incl %eax

# divide by 3
rounded_freq_div_3:
    movl $3, %ebx
    xorl %edx, %edx
    divl %ebx

    # Round if remainder is larger or equal to 1
    cmpl $1, %edx           
    jb get_reload_value
    inc %eax 


# Store the reoload value and calculate the actual frequency
get_reload_value:
    pushl %eax                   # Save reload value
    movw %ax, PIT_reload_value   # Set variable
    movl %eax, %ebx              # EBX = reload value

    # Same calculation to get corrected frequency 
    # Math: Total Hz / frequency = reload value => Total Hz / reload value = frequency
    
    movl $3579545, %eax
    xorl %edx, %edx                   # edx:eax = 3579545
    divl %ebx                         # eax = 3579545 / reload_value, edx = remainder
    cmpl $1789772, %edx               # Is the remainder more than half? 3579545 / 2
    jb rounded_hz_div_3               # no, round down
    incl %eax                         # yes, round up

rounded_hz_div_3:
    movl $3, %ebx
    xorl %edx,%edx                    # edx:eax = 3579545 / reload_value
    divl %ebx                         # eax = (3579545 / 3) / frequency
    cmpl $1, %edx                     # Is the remainder more than half (3/2)?
    jb calc_amount_time               # No, round down
    incl %eax                         # Yes, round up


# Calculate the amount of time between IRQs in 32.32 fixed point
# 
# Note: The basic formula is:
#           time in ms = reload_value / (3579545 / 3) * 1000
#           time in ms = reload_value * 3000 / 3579545
#           time in ms = reload_value * 3000 / 3579545 * (2^42)/(2^42)
#           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^42) * (2^32)
#           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^10)
# 2^42 / 2^10 = 2^32 but because div 3579545 can be innacurate, multiplying a bigger number first is fine.
calc_amount_time:  
    movl %eax, IRQ0_frequency        # Store the actual frequency for displaying later

    # multiply reload value | 3000 * 2^42 / 3579545
    popl %ebx                        # EBX = reload value
    movl $0xDBB3A062, %eax           # EAX = 3000 * (2^42) / 3579545
    mull %ebx                        # EDX:EAX = reload value * (3000 * 2^42 / 3579545)
    
    # divide reload value by 2^10s
    shrd $10, %edx, %eax
    shrl $10, %edx  

    # store them
    movl %edx, IRQ0_ms
    movl %eax, IRQ0_fractions

# Program the PIT channel
program_pit_channel:
    pushf                  # Push flags to make sure nothing gets modified
    cli                    # Disable interrupts

    movb $0b00110100, %al  # channel 0, lobyte/hibyte, Mode 2
    outb %al, $0x43        # output to command

    movw PIT_reload_value, %ax # AX = 16 bit reload value
    outb %al, $0x40            # Output low byte  
    movb %ah, %al              # AL = 8 high bits of reload value
    outb %al, $0x40            # Output high byte

    popf                   # Return flags (including interrupt flag)

# Function end
    popl %eax
    popl %ebx
    popl %edx
    ret