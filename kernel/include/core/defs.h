#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdint.h>
#include <stdbool.h>

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

#define NULL ((void*)0)

#define BIT_TO_BYTE 8

#define STOR_1KiB   0x00000400
#define STOR_2KiB   0x00000800
#define STOR_4KiB   0x00001000
#define STOR_8KiB   0x00002000
#define STOR_16KiB  0x00004000
#define STOR_32KiB  0x00008000
#define STOR_64KiB  0x00010000
#define STOR_128KiB 0x00020000
#define STOR_256KiB 0x00040000
#define STOR_512KiB 0x00080000
#define STOR_1MiB   0x00100000
#define STOR_2MiB   0x00200000
#define STOR_4MiB   0x00400000
#define STOR_8MiB   0x00800000
#define STOR_16MiB  0x01000000
#define STOR_32MiB  0x02000000
#define STOR_64MiB  0x04000000
#define STOR_128MiB 0x08000000
#define STOR_256MiB 0x10000000
#define STOR_512MiB 0x20000000
#define STOR_GiB    0x40000000
#define STOR_2GiB   0x80000000
#define STOR_4GiB   ((uint64_t)0x100000000)

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (uintptr_t)&(((type *)0)->member)))

#define clamp(v, min, max) (v < min ? min : (v > max ? max : v))

#define MAX_VADDR ((uintptr_t)-1) 
#define HIGH_VADDR 0xC0000000

void abort();
void assert(bool must_be_true);

static inline uint32_t log2_u32(uint32_t x) 
{
    uint32_t result;
    __asm__ (
        "bsr %1, %0"
        : "=r" (result)
        : "r" (x)
        : "cc"
    );
    return result;
}


static inline uintptr_t align_to_n(uintptr_t value, uintptr_t alignment/*2^n*/)
{
    assert((alignment & (alignment-1)) == 0);

    return (value + alignment - 1) & ~(alignment - 1);
}
static inline uintptr_t align_up_pow2(uintptr_t value)
{
    if (value == 0) return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

#if UINTPTR_MAX > 0xFFFFFFFFu
    value |= value >> 32;
#endif

    return value;
}

#endif // __DEFS_H__