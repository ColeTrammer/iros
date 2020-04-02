#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/proc/task.h>

static void (*handlers[2 * PIC_IRQS])(void *) = { 0 };
static void *closures[2 * PIC_IRQS];

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

void pic_generic_handler() {
    unsigned int irq_line = 0;
    uint16_t isr = get_isr();

    for (unsigned int i = 0; i < 2 * PIC_IRQS; i++) {
        if (isr & (1 << i) && i != PIC_SLAVE_IRQ) {
            irq_line = i;
            break;
        }
    }
    if (handlers[irq_line] != NULL) {
        handlers[irq_line](closures[irq_line]);
    } else {
        debug_log("Recieved Interrupt on IRQ Line: [ %u ]\n", irq_line);
    }

    sendEOI(irq_line);
}

bool is_irq_line_registered(unsigned int irq_line) {
    return is_irq_registered(irq_line + PIC_IRQ_OFFSET);
}

void register_irq_line_handler(void (*handler)(void *cls), unsigned int irq_line, void *closure, bool use_generic_handler) {
    if (irq_line < 2 * PIC_IRQS) {
        if (use_generic_handler) {
            handlers[irq_line] = handler;
            closures[irq_line] = closure;
            register_irq_handler(&pic_generic_handler_entry, irq_line + PIC_IRQ_OFFSET, false, true);
        } else {
            register_irq_handler(handler, irq_line + PIC_IRQ_OFFSET, false, true);
        }

        enable_irq_line(irq_line);
        debug_log("Registered PIC IRQ Line Handler: [ %#.1X, %#.16lX ]\n", irq_line, (uintptr_t) handler);
    } else {
        printf("Invalid IRQ Line Requested: %u\n", irq_line);
    }
}

void init_pic() {
    remap(PIC_IRQ_OFFSET, PIC_IRQ_OFFSET + PIC_IRQS);
    for (uint8_t i = 0; i < 2 * PIC_IRQS; i++) {
        if (i != PIC_SLAVE_IRQ) {
            disable_irq_line(i);
        }
    }
}