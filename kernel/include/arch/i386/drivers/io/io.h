#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC_EOI		0x20		/* End-of-interrupt command code */
#define ACK 0xFA

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %w1, %b0"
                      : "=a"(ret)
                      : "Nd"(port)
                      : "memory");
    return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile ("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %w1, %w0"
                      : "=a"(ret)
                      : "Nd"(port)
                      : "memory");
    return ret;
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile ("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
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


// Input bytes (uint8_t) array
static inline void insb(uint16_t port, void *addr, uint32_t count)
{
    asm volatile ("rep insb"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output bytes (uint8_t) array
static inline void outsb(uint16_t port, const void *addr, uint32_t count)
{
    asm volatile ("rep outsb"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

// Input words (uint16_t) array
static inline void insw(uint16_t port, void *addr, uint32_t count)
{
    asm volatile ("rep insw"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output words (uint16_t) array
static inline void outsw(uint16_t port, const void *addr, uint32_t count)
{
    asm volatile ("rep outsw"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

// Input double words (uint32_t) array
static inline void insl(uint16_t port, void *addr, uint32_t count)
{
    asm volatile ("rep insl"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

// Output double words (uint32_t) array
static inline void outsl(uint16_t port, const void *addr, uint32_t count)
{
    asm volatile ("rep outsl"
                  : "+S"(addr), "+c"(count)
                  : "d"(port));
}

#endif // __IO_H__