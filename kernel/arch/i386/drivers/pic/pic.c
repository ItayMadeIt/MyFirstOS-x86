#include "core/num_defs.h"
#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pic/pic.h>
#include <kernel/interrupts/irq.h>


#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


static u8 pic_master_base;
static u8 pic_slave_base;

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void pic_remap(int offset1, int offset2)
{
    pic_master_base = (u8)offset1;
    pic_slave_base  = (u8)offset2;

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
    // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	outb(PIC1_DATA, ICW4_8086);               
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PIC1_DATA, 0);
	outb(PIC2_DATA, 0);
}

#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

/* Helper func */
static u16 __pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    u8 irr1 = inb(PIC1_COMMAND);
    u8 irr2 = inb(PIC2_COMMAND);
    return ((u16)irr2 << 8) | irr1;
}

/* Returns the combined value of the cascaded PICs irq request register */
u16 pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}

/* Returns the combined value of the cascaded PICs in-service register */
u16 pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}

void pic_send_eoi_vector(u8 vector)
{
	if (vector >= pic_slave_base && vector < pic_slave_base + 8)
	{
        outb(PIC2, PIC_EOI);
	}
	// then master
	outb(PIC1, PIC_EOI);

}

static void pic_unmask_irq(u8 irq_line)
{
    usize_ptr irq_data = irq_save();

    u8 bit;
    u16 port;

    if (irq_line < 8) 
    {
        port = PIC1_DATA;
        bit  = irq_line;
    } 
    else 
    {
        port = PIC2_DATA;
        bit  = irq_line - 8;

        // ensure cascade (IRQ2) on master is unmasked
        // io_wait();
        // u8 new_mask = inb(PIC1_DATA) & ~(1u << 2);
        // io_wait();
        // outb(PIC1_DATA, new_mask);
    }

    io_wait();
    u8 new_mask = inb(port) & ~(1u << bit);
    io_wait();
    outb(port, new_mask);

    irq_restore(irq_data);
}

static inline int16_t vector_to_irq(u8 vector)
{
    if (vector >= pic_master_base && vector < pic_master_base + 8)
        return (int16_t)(vector - pic_master_base);           // IRQ 0–7
    if (vector >= pic_slave_base  && vector < pic_slave_base  + 8)
        return (int16_t)(8 + (vector - pic_slave_base));      // IRQ 8–15
    return -1; // not a PIC vector
}

void pic_unmask_vector(u8 vector)
{
    int16_t irq = vector_to_irq(vector);

    if (irq >= 0) 
    {
        pic_unmask_irq((u8)irq);
    }
}


void setup_pic()
{
	pic_remap(PIC_IRQ_OFFSET, PIC_IRQ_OFFSET + 8);

	// make everything not usable
	outb(PIC1_DATA, 0xFF ^ (1 << 2)); // only allow slave alerady
	outb(PIC2_DATA, 0xFF);
}