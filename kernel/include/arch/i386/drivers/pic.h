#ifndef __PIC_H__
#define __PIC_H__

#include <stdint.h>
#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */

#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void pic_send_eoi_vector(uint8_t irq);
void setup_pic();

#endif // __PIC_H__