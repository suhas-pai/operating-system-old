/*
 * kernel/cpu/pic.c
 * © suhas pai
 */

#include "asm/pause.h"

#include "port.h"
#include "pic.h"

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC1_CMD                    0x20
#define PIC2_CMD                    0xA0
#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/

void pic_remap(const uint8_t offset1, const uint8_t offset2) {
	unsigned char a1, a2;

	a1 = port_in8(PIC1_DATA);                        // save masks
	a2 = port_in8(PIC2_DATA);

	port_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	cpu_pause();
	port_out8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	cpu_pause();
	port_out8(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	cpu_pause();
	port_out8(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	cpu_pause();
	port_out8(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	cpu_pause();
	port_out8(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	cpu_pause();

	port_out8(PIC1_DATA, ICW4_8086);
	cpu_pause();
	port_out8(PIC2_DATA, ICW4_8086);
	cpu_pause();

	port_out8(PIC1_DATA, a1);   // restore saved masks.
	port_out8(PIC2_DATA, a2);
}