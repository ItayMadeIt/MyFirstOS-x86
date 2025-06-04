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

static inline void PIC_sendEOI(uint8_t irq)
{
    // Sent to the PIC2 End-of-Interrupt as well
	if(irq >= 8)
    {
		outb(PIC2,PIC_EOI);
	}

	outb(PIC1,PIC_EOI);
}
#endif // __IO_H__