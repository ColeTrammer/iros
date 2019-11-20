#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/hal/input.h>
#include <kernel/hal/output.h>

#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pic.h>

struct mouse_event_queue {
    struct mouse_event entry;
    struct mouse_event_queue *next;
};

struct mouse_data {
    bool has_scroll_wheel;
    uint8_t index;
    uint8_t buffer[4];
};

static struct mouse_event_queue *start;
static struct mouse_event_queue *end;
static spinlock_t queue_lock = SPINLOCK_INITIALIZER;

static ssize_t mouse_f_read(struct device *device, struct file *file, void *buffer, size_t len) {
    (void) device;
    (void) file;

    size_t read = 0;
    
    while (read <= len - sizeof(struct mouse_event)) {
        if (start == NULL) {
            return read;
        }

        assert(start);
        while (read <= len - sizeof(struct mouse_event) && start != NULL) {
            memcpy(((uint8_t*) buffer) + read, &start->entry, sizeof(struct mouse_event));

            read += sizeof(struct mouse_event);

            if (start->next == NULL) {
                end = NULL;
            }

            spin_lock(&queue_lock);

            struct mouse_event_queue *save = start->next;
            free(start);
            start = save;

            spin_unlock(&queue_lock);
        }
    }

    return (ssize_t) read;
}


static struct device_ops mouse_ops = {
    NULL, mouse_f_read, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static struct mouse_data data = {
    false, 0, { 0 }
};

static struct device mouse = {
    0x500, S_IFCHR, "mouse", false, &mouse_ops, NULL, &data
};

static void add_mouse_event(struct mouse_event *event) {
    struct mouse_event_queue *e = malloc(sizeof(struct mouse_event_queue));
    memcpy(&e->entry, event, sizeof(struct mouse_event));
    e->next = NULL;

    spin_lock(&queue_lock);

    if (start == NULL) {
        start = e;
        end = e;
    } else {
        end->next = e;
        end = e;
    }

    spin_unlock(&queue_lock);
}

static struct mouse_event s_event = { 0 };

void on_interrupt() {
    uint8_t status = inb(PS2_CONTROL_REGISTER);
    if (!(((status & 0x20) == 0x20) && (status & 0x1))) {
        return;
    }

    uint8_t mouse_data = inb(PS2_DATA_REGISTER);
    switch (data.index) {
        case 0:
            if (!(mouse_data & (1 << 3))) {
                return;
            }
            // Fall through
        case 1:
        case 2:
            data.buffer[data.index++] = mouse_data;
            return;
        case 3:
            if (data.has_scroll_wheel) {
                data.buffer[data.index++] = mouse_data;
                return;
            }
            break;
        case 4:
            assert(data.has_scroll_wheel);
            break;
        default:
            assert(false);
    }

    if (data.buffer[3] == 0x1) {
        s_event.scroll_state = SCROLL_DOWN;
    } else if (data.buffer[3] == 0xFF) {
        s_event.scroll_state = SCROLL_UP;
    } else {
        data.index = 0;
        return;
    }

    add_mouse_event(&s_event);
    data.index = 0;
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