#include <stdio.h>
#include <stdlib.h>

inline static void halt()
{
    asm volatile (
        "cli\n\t"   // disable interrupts
        "hlt\n\t"   // halt the CPU
    );
}

__attribute__((__noreturn__))
void abort(void)
 {
#if defined(__is_libk)
	printf("kernel: panic: abort()\n");
	halt(); // for now just crash 
#else
	// TODO: Abnormally terminate the process as if by SIGABRT.
	printf("abort()\n");
#endif
	while (1) { }
	__builtin_unreachable();
}
