#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/ps2.h>
#include <kernel/irqs/handlers.h>

static int ps2_read(uint8_t *byte) {
    int us_timeout = 300;
    for (int i = 0; i < us_timeout; i++) {
        if (inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) {
            *byte = inb(PS2_IO_DATA);
            return 0;
        }
    }

    return -EIO;
}

static int ps2_out(uint16_t port, uint8_t byte) {
    int us_timeout = 300;
    for (int i = 0; i < us_timeout; i++) {
        if (!(inb(PS2_IO_STATUS) & PS2_STATUS_INPUT_FULL)) {
            outb(port, byte);
            return 0;
        }
    }

    return -EIO;
}

static int ps2_command(uint8_t byte) {
    return ps2_out(PS2_IO_COMMAND, byte);
}

static int ps2_port0_out(uint8_t byte) {
    return ps2_out(PS2_IO_DATA, byte);
}

static int ps2_port1_out(uint8_t byte) {
    if (ps2_command(PS2_COMMAND_WRITE_PORT1) == 0) {
        return ps2_out(PS2_IO_DATA, byte);
    }
    return -EIO;
}

static int ps2_send(uint8_t byte, int port) {
    if (port == 1) {
        return ps2_port1_out(byte);
    }
    return ps2_port0_out(byte);
}

static int ps2_write_config(uint8_t config) {
    if (ps2_command(PS2_COMMAND_WRITE_CONFIG) == 0) {
        return ps2_out(PS2_IO_DATA, config);
    }
    return -EIO;
}

static int ps2_send_device_command(uint8_t byte, int port) {
    uint8_t result;
    if (ps2_send(byte, port) || ps2_read(&result) || result != PS2_ACK) {
        return -EIO;
    }
    return 0;
}

static struct list_node ps2_drivers = INIT_LIST(ps2_drivers);

static struct ps2_driver *ps2_find_driver(struct ps2_controller *controller, int port_number) {
    struct ps2_port *port = &controller->ports[port_number];
    if (ps2_send_device_command(PS2_DEVICE_COMMAND_ID, port_number)) {
        debug_log("Failed to send PS/2 device id command for port: [ %d ]\n", port_number);
        return NULL;
    }

    ps2_read(&port->id.byte0);
    ps2_read(&port->id.byte0);

    list_for_each_entry(&ps2_drivers, driver, struct ps2_driver, list) {
        for (size_t i = 0; i < driver->device_id_count; i++) {
            struct ps2_device_id id = driver->device_id_list[i];
            if (memcmp(&port->id, &id, sizeof(id)) == 0) {
                return driver;
            }
        }
    }

    debug_log("Can't find PS/2 device driver for port: [ %d, %u, %u ]\n", port_number, port->id.byte0, port->id.byte1);
    return NULL;
}

void ps2_register_driver(struct ps2_driver *driver) {
    debug_log("Registering PS/2 driver: [ %s ]\n", driver->name);
    list_append(&ps2_drivers, &driver->list);
}

void ps2_unregister_driver(struct ps2_driver *driver) {
    debug_log("Unregistering PS/2 driver: [ %s ]\n", driver->name);
    list_remove(&driver->list);
}

void init_ps2_controller(void) {
    // Disable PS/2 ports
    if (ps2_command(PS2_COMMAND_DISABLE_PORT0) || ps2_command(PS2_COMMAND_DISABLE_PORT1)) {
        debug_log("PS/2 controller is broken\n");
        return;
    }

    // Discard any pending data
    inb(PS2_IO_DATA);

    // Modify the config register so that IRQs are disabled.
    uint8_t config;
    if (ps2_command(PS2_COMMAND_READ_CONFIG) || ps2_read(&config)) {
        debug_log("PS/2 controller can't read config byte\n");
        return;
    }
    bool no_port1 = !(config & PS2_CONFIG_CLOCK1_DISABLED);
    config &= ~(PS2_CONFIG_IRQ0_ENABLED | PS2_CONFIG_IRQ1_ENABLED);
    config |= PS2_CONFIG_TRANLATE;
    if (ps2_write_config(config)) {
        debug_log("PS/2 controller can't write config byte\n");
        return;
    }

    // Do PS/2 self test
    uint8_t result;
    if (ps2_command(PS2_COMMAND_TEST_CONTROLLER) || ps2_read(&result) || result != PS2_CONTROLLER_SELF_TEST_SUCCESS) {
        debug_log("PS/2 controller self test failed\n");
        return;
    }
    if (ps2_write_config(config)) {
        debug_log("PS/2 controller can't write config byte\n");
        return;
    }

    // Check for the second PS/2 port
    bool has_port1 = false;
    if (!no_port1) {
        if (ps2_command(PS2_COMMAND_ENABLE_PORT1)) {
            debug_log("PS/2 controller can't enable port 1\n");
            return;
        }

        if (ps2_command(PS2_COMMAND_READ_CONFIG) || ps2_read(&config)) {
            debug_log("PS/2 controller can't read config byte\n");
            return;
        }

        if (!(config & PS2_CONFIG_CLOCK1_DISABLED)) {
            has_port1 = true;
            ps2_command(PS2_COMMAND_DISABLE_PORT1);
        }
    }

    // Check if the ports work
    bool has_port0 = true;
    if (ps2_command(PS2_COMMAND_TEST_PORT0) || ps2_read(&result) || result != 0x00) {
        debug_log("PS/2 port 0 is broken\n");
        has_port0 = false;
    }
    if (has_port1) {
        if (ps2_command(PS2_COMMAND_TEST_PORT1) || ps2_read(&result) || result != 0x00) {
            debug_log("PS/2 port 1 is broken\n");
            has_port1 = false;
        }
    }
    if (!has_port0 && !has_port1) {
        debug_log("PS/2 controller has no working ports\n");
        return;
    }

    struct ps2_controller *controller = calloc(1, sizeof(struct ps2_controller));
    controller->read_byte = ps2_read;
    controller->send_byte = ps2_send;
    controller->send_command = ps2_send_device_command;
    controller->ports[0].port_number = 0;
    controller->ports[0].irq = PS2_IRQ0 + EXTERNAL_IRQ_OFFSET;
    controller->ports[1].port_number = 1;
    controller->ports[1].irq = PS2_IRQ1 + EXTERNAL_IRQ_OFFSET;

    // Enable ports, reset devices, and locate drivers
    struct ps2_driver *driver0 = NULL;
    struct ps2_driver *driver1 = NULL;
    if (has_port0) {
        if (ps2_command(PS2_COMMAND_ENABLE_PORT0) || ps2_send_device_command(PS2_DEVICE_COMMAND_SELF_TEST, 0) || ps2_read(&result) ||
            result != PS2_DEVICE_SELF_TEST_SUCCESS ||
            (ps2_read(&result), ps2_send_device_command(PS2_DEVICE_COMMAND_DISABLE_SCANNING, 0))) {
            debug_log("PS/2 device on port 0 is broken\n");
            has_port0 = false;
            ps2_command(PS2_COMMAND_DISABLE_PORT0);
        } else if (!(driver0 = ps2_find_driver(controller, 0))) {
            has_port0 = false;
            ps2_command(PS2_COMMAND_DISABLE_PORT0);
        }
    }
    if (has_port1) {
        if (ps2_command(PS2_COMMAND_ENABLE_PORT1) || ps2_send_device_command(PS2_DEVICE_COMMAND_SELF_TEST, 1) || ps2_read(&result) ||
            result != PS2_DEVICE_SELF_TEST_SUCCESS ||
            (ps2_read(&result), ps2_send_device_command(PS2_DEVICE_COMMAND_DISABLE_SCANNING, 1))) {
            debug_log("PS/2 device on port 1 is broken\n");
            ps2_command(PS2_COMMAND_DISABLE_PORT1);
            has_port1 = false;
        } else if (!(driver1 = ps2_find_driver(controller, 1))) {
            has_port1 = false;
            ps2_command(PS2_COMMAND_DISABLE_PORT1);
        }
    }

    if (!has_port0 && !has_port1) {
        free(controller);
        return;
    }

    // Start the device drivers
    if (driver0) {
        driver0->create(controller, &controller->ports[0]);
    }
    if (driver1) {
        driver1->create(controller, &controller->ports[1]);
    }

    // Enable IRQs
    ps2_command(PS2_COMMAND_READ_CONFIG);
    ps2_read(&config);
    config |= (has_port0 ? PS2_CONFIG_IRQ0_ENABLED : 0) | (has_port1 ? PS2_CONFIG_IRQ1_ENABLED : 0);
    ps2_write_config(config);
}
