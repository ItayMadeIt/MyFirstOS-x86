#ifndef __IO_H__
#define __IO_H__

#include "core/num_defs.h"

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC_EOI		0x20		/* End-of-interrupt command code */
#define ACK 0xFA

static inline void outb(u16 port, u8 val)
{
    asm volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u8 inb(u16 port)
{
    u8 ret;
    asm volatile ("inb %w1, %b0"
                      : "=a"(ret)
                      : "Nd"(port)
                      : "memory");
    return ret;
}

static inline void outw(u16 port, u16 val)
{
    asm volatile ("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u16 inw(u16 port)
{
    u16 ret;
    asm volatile ("inw %w1, %w0"
                      : "=a"(ret)
                      : "Nd"(port)
                      : "memory");
    return ret;
}

static inline void outl(u16 port, u32 val)
{
    asm volatile ("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline u32 inl(u16 port)
{
    u32 ret;
    asm volatile ("inl %w1, %0"
                      : "=a"(ret)
                      : "Nd"(port)
                      : "memory");
    return ret;
}

static inline void io_wait()
{
    outb(0x80, 0);
}


// Input bytes (u8) array
static inline void insb(u16 port, void *addr, u32 count)
{
    asm volatile ("rep insb"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output bytes (u8) array
static inline void outsb(u16 port, const void *addr, u32 count)
{
    asm volatile ("rep outsb"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

// Input words (u16) array
static inline void insw(u16 port, void *addr, u32 count)
{
    asm volatile ("rep insw"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output words (u16) array
static inline void outsw(u16 port, const void *addr, u32 count)
{
    asm volatile ("rep outsw"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

// Input double words (u32) array
static inline void insl(u16 port, void *addr, u32 count)
{
    asm volatile ("rep insl"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output double words (u32) array
static inline void outsl(u16 port, const void *addr, u32 count)
{
    asm volatile ("rep outsl"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

#endif // __IO_H__