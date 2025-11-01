#include <string.h>

usize strlen(const char* str) {
	usize len = 0;
	while (str[len])
		len++;
	return len;
}
