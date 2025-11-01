#include <stdio.h>

#if defined(__is_libk)
#include <drivers/tty.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	terminal_write(&c, (usize_ptr)sizeof(c));
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
