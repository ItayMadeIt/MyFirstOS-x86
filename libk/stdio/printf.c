#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) 
{
	const unsigned char* bytes = (const unsigned char*) data;

	for (size_t i = 0; i < length; i++)
	{	
		if (putchar(bytes[i]) == EOF)
			return false;
	}
	
	return true;
}

static int print_int(int value) 
{
    if (value == 0) 
	{
        putchar('0');
        return 1;
    }

	int count = 0;

    if (value < 0) 
	{
        putchar('-');
        value = -value;

		count++;
    }

    char digits[32]; // enough for even a 64 bit int 2^64 is ~20 digits
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = '0' + (value % 10);
        value /= 10;
    }

    // print in reverse order
    while (i--) 
	{
        if (putchar(digits[i]) == EOF)
		{
			return -1;
		}

		count++;
    }
	return count;
}

static int print_long(long value) 
{
    if (value == 0) 
	{
        putchar('0');
        return 1;
    }

	int count = 0;

    if (value < 0) 
	{
        putchar('-');
        value = -value;

		count++;
    }

    char digits[32]; // enough for even a 64 bit int 2^64 is ~20 digits
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = '0' + (value % 10);
        value /= 10;
    }

    // print in reverse order
    while (i--) 
	{
        if (putchar(digits[i]) == EOF)
		{
			return -1;
		}

		count++;
    }
	return count;
}


static int print_longlong(long long value) 
{
    if (value == 0) 
	{
        putchar('0');
        return 1;
    }

	int count = 0;

    if (value < 0) 
	{
        putchar('-');
        value = -value;

		count++;
    }

    char digits[32]; // enough for even a 64 bit int 2^64 is ~20 digits
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = '0' + (value % 10);
        value /= 10;
    }

    // print in reverse order
    while (i--) 
	{
        if (putchar(digits[i]) == EOF)
		{
			return -1;
		}

		count++;
    }
	return count;
}

static int print_ulong(unsigned long value) 
{
    if (value == 0) 
	{
        putchar('0');
        return 1;
    }

    char digits[32]; // enough for even a 64 bit int 2^64 is ~20 digits
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = '0' + (value % 10);
        value /= 10;
    }

	int count = 0;

    // print in reverse order
    while (i--) 
	{
        if (putchar(digits[i]) == EOF)
		{
			return -1;
		}

		count++;
    }
	return count;
}

static int print_ulonglong(unsigned long long value) 
{
    if (value == 0) 
	{
        putchar('0');
        return 1;
    }

    char digits[32]; // enough for even a 64 bit int 2^64 is ~20 digits
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = '0' + (value % 10);
        value /= 10;
    }

	int count = 0;

    // print in reverse order
    while (i--) 
	{
        if (putchar(digits[i]) == EOF)
		{
			return -1;
		}

		count++;
    }
	return count;
}

static int print_hex(unsigned int value, bool uppercase) 
{
    if (value == 0) 
	{
		if (!print("0x0", sizeof("0x0")-1))
		{
			return -1;
		}

        return sizeof("0x0");
    }

	if (!print("0x", sizeof("0x")-1))
	{
		return -1;
	}
	int count = sizeof("0x");

    const char* hex = uppercase ? 
		"0123456789ABCDEF" : "0123456789abcdef";
    char digits[16];
    int i = 0;

    while (value > 0) 
	{
        digits[i++] = hex[value % 16];
        value /= 16;
    }

    while (i--) 
	{
        putchar(digits[i]);
		count++;
	}

	return count;
}


int printf(const char* restrict format, ...) 
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') 
	{
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') 
		{
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) 
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') 
		{
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} 
		else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == 'd') {
			format++;
            int val = va_arg(parameters, int);
            int count = print_int(val);
			if (count == -1)
				return -1;
			written += count;
		} else if (*format == 'x') {
			format++;
            int val = va_arg(parameters, unsigned int);
            int count = print_hex(val, false);
			if (count == -1)
				return -1;
			written += count;
		}  else if (*format == 'X') {
			format++;
            int val = va_arg(parameters, unsigned int);
            int count = print_hex(val, true);
			if (count == -1)
				return -1;
			written += count;
		}
		else if (*format == 'l') {
			format++; // skip first 'l'

			if (*format == 'd') {
				format++;
				long val = va_arg(parameters, long);
				int count = print_long(val);
				if (count == -1) return -1;
				written += count;
			}
			else if (*format == 'u') {
				format++;
				unsigned long val = va_arg(parameters, unsigned long);
				int count = print_ulong(val);
				if (count == -1) return -1;
				written += count;
			}
			else if (*format == 'l') {
			
				format++; // saw second 'l'
				if (*format == 'd') { // %lld
					format++;
					long long val = va_arg(parameters, long long);
					int count = print_longlong(val);
					if (count == -1) return -1;
					written += count;
				}
				else if (*format == 'u') { // %llu
					format++;
					unsigned long long val = va_arg(parameters, unsigned long long);
					int count = print_ulonglong(val);
					if (count == -1) return -1;
					written += count;
				}
				else if (*format == 'x') { // %llx
					format++;
					unsigned long long val = va_arg(parameters, unsigned long long);
					// extend your hex printer to 64-bit:
					char buf[32];
					int i = 0;
					const char* hex = "0123456789abcdef";
					if (val == 0) {
						putchar('0');
						written++;
					} else {
						while (val > 0) {
							buf[i++] = hex[val % 16];
							val /= 16;
						}
						while (i--) {
							putchar(buf[i]);
							written++;
						}
					}
				}
				else if (*format == 'X') { // %llX
					format++;
					unsigned long long val = va_arg(parameters, unsigned long long);
					char buf[32];
					int i = 0;
					const char* hex = "0123456789ABCDEF";
					if (val == 0) {
						putchar('0');
						written++;
					} else {
						while (val > 0) {
							buf[i++] = hex[val % 16];
							val /= 16;
						}
						while (i--) {
							putchar(buf[i]);
							written++;
						}
					}
				}
			}
		}
		else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
