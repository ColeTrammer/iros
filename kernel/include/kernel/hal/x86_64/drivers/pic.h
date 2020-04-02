#ifndef _KERNEL_HAL_X86_64_DRIVERS_PIC_H
#define _KERNEL_HAL_X86_64_DRIVERS_PIC_H 1

#include <stdbool.h>
#include <stdint.h>

#define PIC1         0x20 /* IO base address for master PIC */
#define PIC2         0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)

#define PIC_READ_IRR 0x0a /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR 0x0b /* OCW3 irq service next CMD read */
#define PIC_EOI      0x20

#define ICW1_ICW4      0x01 /* ICW4 (not) needed */
#define ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define ICW1_INIT      0x10 /* Initialization - required! */

#define ICW4_8086       0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM       0x10 /* Special fully nested (not) */

#define PIC_SLAVE_IRQ 2

#define PIC_IRQS       8
#define PIC_IRQ_OFFSET 0x20

void init_pic();

void pic_generic_handler_entry();

bool is_irq_line_registered(unsigned int irq_line);
void enable_irq_line(uint8_t irq_line);
void disable_irq_line(uint8_t irq_line);

void sendEOI(unsigned int irq_line);
void register_irq_line_handler(void (*handler)(void *), unsigned int irq_line, void *closure, bool use_generic_handler);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_PIC_H */