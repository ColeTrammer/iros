#include <stdio.h>
#include <stdbool.h>

#include <kernel/hal/output.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/arch/x86_64/asm_utils.h>

static void handle_serial_interrupt() {
    debug_log("Recieved Serial Port Interrupt: Status [ %#.2X ]\n", inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)));
}

static void serial_write_character(char c) {
    while ((inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)) & SERIAL_TRANSMITTER_EMPTY_BUFFER) == 0);

    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_DATA_OFFSET), c);
}

bool serial_write_message(const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        serial_write_character(s[i]);
    }

    return true;
}

void init_serial_ports() {
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_INTERRUPT_OFFSET), 0);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_CONTROL_OFFSET), SERIAL_DLAB);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_BAUD_LOW_OFFSET), SERIAL_BAUD_DIVISOR & 0xFF);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_BAUD_HIGH_OFFSET), SERIAL_BAUD_DIVISOR >> 8);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_CONTROL_OFFSET), SERIAL_STOP_1_BIT | SERIAL_PARITY_NONE | SERIAL_8_BITS);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_FIFO_OFFSET), 0xC7);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_INTERRUPT_OFFSET), SERIAL_INTERRUPT_DATA | SERIAL_INTERRUPT_ERROR | SERIAL_INTERRUPT_STATUS);

    register_irq_line_handler(&handle_serial_interrupt, SERIAL_13_IRQ_LINE, true);
    debug_log("Serial Port Initialized: [ %#.3X ]\n", SERIAL_COM1_PORT);
}