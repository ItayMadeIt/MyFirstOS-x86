
#include "arch/i386/boot/early.h"
#include <drivers/tty.h>

int early_putchar(int ic) 
{
    void (*early_terminal_write)(const char*, usize_ptr) =
        (void (*)(const char*, usize_ptr)) virt_to_phys_code( (uptr)(void*)terminal_write);

    char ch = ic;
    
    early_terminal_write(&ch, 1);
    
	return ic;
}
