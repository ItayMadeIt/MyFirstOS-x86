#include "kernel/core/cpu.h"
#include <stdio.h>
#include <stdlib.h>

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
	while (1) 
	{ 
		cpu_halt();	
	}
	__builtin_unreachable();
}
