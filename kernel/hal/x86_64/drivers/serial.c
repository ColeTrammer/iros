#include <stdio.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/arch/x86_64/asm_utils.h>

void handle_serial_interrupt() {
    printf("%#.2X\n", inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)));
}

void serial_write_character(char c) {
    while ((inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)) & SERIAL_TRANSMITTER_EMPTY_BUFFER) == 0);

    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_DATA_OFFSET), c);
}

void serial_write_message(const char *s) {
    while (*s != '\0') {
        serial_write_character(*s);
        s++;
    }
}

void init_serial_ports() {
    register_irq_line_handler(&handle_serial_interrupt, SERIAL_13_IRQ_LINE);

    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_INTERRUPT_OFFSET), 0);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_CONTROL_OFFSET), SERIAL_DLAB);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_BAUD_LOW_OFFSET), SERIAL_BAUD_DIVISOR & 0xFF);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_BAUD_HIGH_OFFSET), SERIAL_BAUD_DIVISOR >> 8);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_CONTROL_OFFSET), SERIAL_STOP_1_BIT | SERIAL_PARITY_NONE | SERIAL_8_BITS);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_FIFO_OFFSET), 0xC7);
    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_INTERRUPT_OFFSET), SERIAL_INTERRUPT_DATA | SERIAL_INTERRUPT_ERROR | SERIAL_INTERRUPT_STATUS);

    serial_write_message("Serial Port Initialized...\n");
}