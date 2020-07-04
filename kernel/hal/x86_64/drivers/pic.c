#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/task.h>

void sendEOI(unsigned int irq_line) {
    if (irq_line >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

static void remap(int offset1, int offset2) {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void enable_irq_line(uint8_t irq_line) {
    uint16_t port;
    uint8_t val;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    val = inb(port) & ~(1 << irq_line);
    outb(port, val);
}

void disable_irq_line(uint8_t irq_line) {
    uint16_t port;
    uint8_t val;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    val = inb(port) | (1 << irq_line);
    outb(port, val);
}

static uint16_t get_irq_reg(int ocw3) {
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

static uint16_t get_isr() {
    return get_irq_reg(PIC_READ_ISR);
}

static bool pic_is_valid_irq(struct irq_controller *self, int irq_num) {
    (void) self;
    int adjusted_irq = irq_num - PIC_IRQ_OFFSET;
    uint16_t isr = get_isr();
    return !!(isr & (1 << adjusted_irq));
}

static void pic_send_eoi(struct irq_controller *self, int irq_num) {
    (void) self;
    sendEOI(irq_num - PIC_IRQ_OFFSET);
}

static void pic_set_irq_enabled(struct irq_controller *self, int irq_num, bool enabled) {
    (void) self;

    int adjusted_irq = irq_num - PIC_IRQ_OFFSET;
    if (enabled) {
        enable_irq_line(adjusted_irq);
    } else {
        disable_irq_line(adjusted_irq);
    }
}

static struct irq_controller_ops pic_ops = { .is_valid_irq = &pic_is_valid_irq,
                                             .send_eoi = &pic_send_eoi,
                                             .set_irq_enabled = &pic_set_irq_enabled };

static struct irq_controller pic = { .irq_start = PIC_IRQ_OFFSET, .irq_end = PIC_IRQ_OFFSET + 2 * PIC_IRQS - 1, .ops = &pic_ops };

void init_pic() {
    remap(PIC_IRQ_OFFSET, PIC_IRQ_OFFSET + PIC_IRQS);
    for (uint8_t i = 0; i < 2 * PIC_IRQS; i++) {
        if (i != PIC_SLAVE_IRQ) {
            disable_irq_line(i);
        }
    }

    register_irq_controller(&pic);
}

void disable_pic(void) {
    // Remap the pic irqs out of the way, and then disable all irqs the PIC can generate.
    remap(PIC_IRQ_OFFSET, PIC_IRQ_OFFSET + PIC_IRQS);
    for (uint8_t i = 0; i < 2 * PIC_IRQS; i++) {
        disable_irq_line(i);
    }
}
