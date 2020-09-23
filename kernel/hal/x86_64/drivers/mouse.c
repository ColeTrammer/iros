#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/hal/input.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>

#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/ps2.h>
#include <kernel/hal/x86_64/drivers/vmware_back_door.h>

struct mouse_event_queue {
    struct mouse_event entry;
    struct mouse_event_queue *next;
};

struct mouse_data {
    struct mouse_event_queue *start;
    struct mouse_event_queue *end;
    spinlock_t queue_lock;
    struct ps2_controller *controller;
    struct ps2_port *port;
    struct device *device;
    struct irq_handler irq_handler;
    struct mouse_event event;
    bool left_is_down;
    bool right_is_down;
    bool has_scroll_wheel;
    uint8_t index;
    uint8_t buffer[4];
};

static ssize_t mouse_f_read(struct device *device, off_t offset, void *buffer, size_t len, bool non_blocking);

static struct device_ops mouse_ops = { .read = mouse_f_read };

static ssize_t mouse_f_read(struct device *device, off_t offset, void *buffer, size_t len, bool non_blocking) {
    (void) offset;
    (void) non_blocking;

    size_t read = 0;

    struct mouse_data *data = device->private;

    while (read <= len - sizeof(struct mouse_event)) {
        if (data->start == NULL) {
            return read;
        }

        assert(data->start);
        while (read <= len - sizeof(struct mouse_event) && data->start != NULL) {
            memcpy(((uint8_t *) buffer) + read, &data->start->entry, sizeof(struct mouse_event));

            read += sizeof(struct mouse_event);

            if (data->start->next == NULL) {
                data->end = NULL;
            }

            spin_lock(&data->queue_lock);

            struct mouse_event_queue *save = data->start->next;
            free(data->start);
            data->start = save;

            device->readable = false;
            spin_unlock(&data->queue_lock);
        }
    }

    return (ssize_t) read;
}

static void add_mouse_event(struct mouse_data *data, struct mouse_event *event) {
    struct mouse_event_queue *e = malloc(sizeof(struct mouse_event_queue));
    memcpy(&e->entry, event, sizeof(struct mouse_event));
    e->next = NULL;

    spin_lock(&data->queue_lock);

    if (data->start == NULL) {
        data->start = e;
        data->end = e;
    } else {
        data->end->next = e;
        data->end = e;
    }

    data->device->readable = true;
    spin_unlock(&data->queue_lock);
}

void on_interrupt(struct irq_context *context) {
    struct device *device = context->closure;
    struct mouse_data *data = device->private;

    uint8_t mouse_data;
    if (data->controller->read_byte(&mouse_data)) {
        return;
    }

    if (vmmouse_is_enabled()) {
        struct mouse_event *event = vmmouse_read();
        if (event) {
            add_mouse_event(data, event);
        }
        return;
    }

    switch (data->index) {
        case 0:
            if (!(mouse_data & (1 << 3))) {
                return;
            }
            // Fall through
        case 1:
        case 2:
            data->buffer[data->index++] = mouse_data;
            if (data->has_scroll_wheel) {
                return;
            }
            break;
        case 3:
            assert(data->has_scroll_wheel);
            data->buffer[data->index++] = mouse_data;
            break;
        default:
            assert(false);
    }

    if (data->buffer[3] == 0x1) {
        data->event.scroll_state = SCROLL_DOWN;
    } else if (data->buffer[3] == 0xFF) {
        data->event.scroll_state = SCROLL_UP;
    }

    if (data->buffer[0] & 0x1) {
        if (data->left_is_down) {
            data->event.left = MOUSE_NO_CHANGE;
        } else {
            data->event.left = MOUSE_DOWN;
            data->left_is_down = true;
        }
    } else {
        if (!data->left_is_down) {
            data->event.left = MOUSE_NO_CHANGE;
        } else {
            data->event.left = MOUSE_UP;
            data->left_is_down = false;
        }
    }

    if (data->buffer[0] & 0x2) {
        if (data->right_is_down) {
            data->event.right = MOUSE_NO_CHANGE;
        } else {
            data->event.right = MOUSE_DOWN;
            data->right_is_down = true;
        }
    } else {
        if (!data->right_is_down) {
            data->event.right = MOUSE_NO_CHANGE;
        } else {
            data->event.right = MOUSE_UP;
            data->right_is_down = false;
        }
    }

    if (data->buffer[0] & 0xC0) {
        data->event.dx = 0;
        data->event.dy = 0;
    } else {
        data->event.dx = (int) data->buffer[1] - ((((int) data->buffer[0]) << 4) & 0x100);
        data->event.dy = (int) data->buffer[2] - ((((int) data->buffer[0]) << 3) & 0x100);
    }

    data->event.scale_mode = SCALE_NONE;
    add_mouse_event(data, &data->event);
    data->index = 0;
}

static void mouse_create(struct ps2_controller *controller, struct ps2_port *port) {
    struct device *device = calloc(1, sizeof(struct device) + sizeof(struct mouse_data));
    struct mouse_data *data = device->private = device + 1;
    device->device_number = 0x00702;
    device->ops = &mouse_ops;
    device->type = S_IFCHR;
    dev_register(device);

    init_spinlock(&data->queue_lock);
    data->controller = controller;
    data->port = port;
    data->device = device;
    data->irq_handler.closure = device;
    data->irq_handler.flags = IRQ_HANDLER_EXTERNAL;
    data->irq_handler.handler = on_interrupt;
    register_irq_handler(&data->irq_handler, port->irq);

    controller->send_command(PS2_DEVICE_COMMAND_SET_DEFAULTS, port->port_number);

    controller->send_command(PS2_DEVICE_COMMAND_SET_SAMPLE_RATE, port->port_number);
    controller->send_command(200, port->port_number);

    controller->send_command(PS2_DEVICE_COMMAND_SET_SAMPLE_RATE, port->port_number);
    controller->send_command(100, port->port_number);

    controller->send_command(PS2_DEVICE_COMMAND_SET_SAMPLE_RATE, port->port_number);
    controller->send_command(80, port->port_number);

    controller->send_command(PS2_DEVICE_COMMAND_ID, port->port_number);

    uint8_t mouse_id = 0xFF;
    uint8_t scratch;
    controller->read_byte(&mouse_id);
    controller->read_byte(&scratch);
    controller->read_byte(&scratch);
    debug_log("Mouse id: [ %u ]\n", mouse_id);

    data->has_scroll_wheel = mouse_id == 3;
    controller->send_command(PS2_DEVICE_COMMAND_ENABLE_SCANNING, port->port_number);
}

static struct ps2_device_id mouse_ids[] = {
    { 0x00, 0x00 },
    { 0x03, 0x00 },
    { 0x04, 0x00 },
};

static struct ps2_driver mouse_driver = {
    .name = "PS/2 Mouse",
    .device_id_list = mouse_ids,
    .device_id_count = sizeof(mouse_ids) / sizeof(struct ps2_device_id),
    .create = mouse_create,
};

void init_mouse() {
    ps2_register_driver(&mouse_driver);
}
