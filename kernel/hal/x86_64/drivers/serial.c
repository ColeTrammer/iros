#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/fs/dev.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/irqs/handlers.h>
#include <kernel/util/init.h>

static bool handle_serial_interrupt(struct irq_context *context __attribute__((unused))) {
    debug_log("Recieved Serial Port Interrupt: Status [ %#.2X ]\n", inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)));
    return true;
}

static void serial_write_character(char c) {
    while ((inb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_STATUS_OFFSET)) & SERIAL_TRANSMITTER_EMPTY_BUFFER) == 0)
        ;

    outb(SERIAL_PORT(SERIAL_COM1_PORT, SERIAL_DATA_OFFSET), c);
}

bool serial_write_message(const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
#ifndef KERNEL_NO_DEBUG_COLORS
        if ((i == 0 || s[i - 1] != '\033') && s[i] == '[') {
            serial_write_message("\033[33m", 5);
        }
#endif /* KERNEL_NO_DEBUG_COLORS */

        serial_write_character(s[i]);

#ifndef KERNEL_NO_DEBUG_COLORS
        if (s[i] == ']') {
            serial_write_message("\033[0m", 5);
        }
#endif /* KERNEL_NO_DEBUG_COLORS */
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
    debug_log("Serial Port Initialized: [ %#.3X ]\n", SERIAL_COM1_PORT);
}

static ssize_t serial_write(struct fs_device *device, off_t offset, const void *buffer, size_t len, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) non_blocking;

    if (!serial_write_message(buffer, len)) {
        return -EIO;
    }
    return (ssize_t) len;
}

static struct fs_device_ops serial_ops = {
    .write = &serial_write,
};

static struct irq_handler serial_handler = {
    .handler = &handle_serial_interrupt,
    .flags = IRQ_HANDLER_EXTERNAL,
};

static void init_serial_port_device(struct hw_device *parent, dev_t port, size_t i) {
    /* Could be anything */
    assert(port == SERIAL_COM1_PORT);

    register_irq_handler(&serial_handler, SERIAL_13_IRQ_LINE + EXTERNAL_IRQ_OFFSET);

    struct fs_device *device = calloc(1, sizeof(struct fs_device));
    device->device_number = 0x00800 + i;
    device->ops = &serial_ops;
    device->mode = S_IFCHR | 0666;
    device->private = NULL;
    snprintf(device->name, sizeof(device->name) - 1, "serial%lu", i);

    struct hw_device *hw = create_hw_device("Serial Port", parent, hw_device_id_isa(), device);
    hw->status = HW_STATUS_ACTIVE;
}

static void detect_serial_ports(struct hw_device *parent) {
    init_serial_port_device(parent, SERIAL_COM1_PORT, 0);
}

static struct isa_driver serial_port_driver = {
    .name = "x86 Serial Port",
    .detect_devices = detect_serial_ports,
};

static void init_serial_port_driver(void) {
    register_isa_driver(&serial_port_driver);
}
INIT_FUNCTION(init_serial_port_driver, driver);
