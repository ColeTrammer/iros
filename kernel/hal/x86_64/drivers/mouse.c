#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/hal/input.h>
#include <kernel/hal/output.h>

#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pic.h>

struct mouse_data {
    bool has_scroll_wheel;
};

static struct device_ops mouse_ops = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static struct mouse_data data = {
    false
};

static struct device mouse = {
    0x500, S_IFCHR, "mouse", false, &mouse_ops, &data
};

void on_interrupt() {
    uint8_t status = inb(PS2_CONTROL_REGISTER);
    if (!(((status & 0x20) == 0x20) && (status & 0x1))) {
        return;
    }

    uint8_t data = inb(PS2_DATA_REGISTER);
    debug_log("Data byte: [ %u ]\n", data);
}

void init_mouse() {
    register_irq_line_handler(on_interrupt, MOUSE_IRQ_LINE_NUM, true);

    mouse_wait_for_output();
    outb(PS2_CONTROL_REGISTER, 0xA8);

    mouse_send_command(MOUSE_COMMAND_SINGLE_PACKET);
    uint8_t ack = mouse_read();
    if (ack == MOUSE_ACK) {
        debug_log("Mouse detected\n");

        mouse_read();
        mouse_read();
        mouse_read();
    } else {
        debug_log("No mouse detected\n");
        return;
    }

    mouse_wait_for_output();
    outb(PS2_CONTROL_REGISTER, MOUSE_COMMAND_GET_COMPAQ);
    uint8_t status = mouse_read();
    status |= 2;

    mouse_wait_for_output();
    outb(PS2_CONTROL_REGISTER, MOUSE_COMMAND_SET_COMPAQ);
    mouse_wait_for_output();
    outb(PS2_DATA_REGISTER, status);

    mouse_send_command(MOUSE_COMMAND_SET_DEFAULTS);
    assert(mouse_read() == MOUSE_ACK);

    mouse_send_command(MOUSE_COMMAND_ENABLE_PACKETS);
    assert(mouse_read() == MOUSE_ACK);

    mouse_send_command(MOUSE_COMMAND_SET_SAMPLE_RATE);
    assert(mouse_read() == MOUSE_ACK);
    mouse_send_command(200);
    assert(mouse_read() == MOUSE_ACK);

    mouse_send_command(MOUSE_COMMAND_SET_SAMPLE_RATE);
    assert(mouse_read() == MOUSE_ACK);
    mouse_send_command(100);
    assert(mouse_read() == MOUSE_ACK);

    mouse_send_command(MOUSE_COMMAND_SET_SAMPLE_RATE);
    assert(mouse_read() == MOUSE_ACK);
    mouse_send_command(80);
    assert(mouse_read() == MOUSE_ACK);

    mouse_send_command(MOUSE_COMMAND_ID);
    assert(mouse_read() == MOUSE_ACK);

    uint8_t mouse_id = mouse_read();
    debug_log("Mouse id: [ %u ]\n", mouse_id);

    data.has_scroll_wheel = mouse_id == 3;

    dev_add(&mouse, mouse.name);
}