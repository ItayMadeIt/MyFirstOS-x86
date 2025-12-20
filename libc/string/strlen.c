#include <string.h>

usize_ptr strlen(const char* str) {
	usize_ptr len = 0;
	while (str[len])
		len++;
	return len;
}
