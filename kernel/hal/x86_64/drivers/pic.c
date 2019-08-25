#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/hal/irqs.h>

#include "io.h"
#include "pic.h"

static void sendEOI(unsigned int irq_line) {
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

static void enable_irq_line(uint8_t irq_line) {
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

static void disable_irq_line(uint8_t irq_line) {
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

static void (**handlers)(void);

void register_irq_line_handler(void (*handler)(void), unsigned int irq_line) {
    if (irq_line < 2 * PIC_IRQS) {
        handlers[irq_line] = handler;
    } else {
        printf("Invalid IRQ Line Requested: %u\n", irq_line);
    }
}

void pic_generic_handler() {
    unsigned int irq_line = 0;
    uint16_t isr = get_isr();
    for (unsigned int i = 0; i < 2 * PIC_IRQS; i++) {
        if (isr & (1 << i)) {
            irq_line = i;
            break;
        }
    }
    printf("Recieved Interrupt on IRQ Line: %u\n", irq_line);
    if (handlers[irq_line] != NULL) {
        handlers[irq_line]();
    }
    sendEOI(irq_line);
}


void init_pic() {
    remap(PIC_IRQ_OFFSET, PIC_IRQ_OFFSET + PIC_IRQS);
    for (uint8_t i = 0; i < 2 * PIC_IRQS; i++) {
        enable_irq_line(i);
    }
    disable_irq_line(0);
    disable_irq_line(1);
    for (unsigned int i = PIC_IRQ_OFFSET; i < PIC_IRQ_OFFSET + 2 * PIC_IRQS; i++) {
        register_irq_handler(&pic_generic_handler_entry, i);
    }
    handlers = calloc(2 * PIC_IRQS, sizeof(void (*)(void)));
}