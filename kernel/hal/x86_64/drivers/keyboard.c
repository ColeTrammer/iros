#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/fs/dev.h>
#include <kernel/hal/input.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/ps2.h>
#include <kernel/irqs/handlers.h>
#include <kernel/net/umessage.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

struct kbd_data {
    struct keyboard_event_queue *start;
    struct keyboard_event_queue *end;
    struct ps2_controller *controller;
    struct ps2_port *port;
    struct device *device;
    struct irq_handler irq_handler;
    struct key_event event;
    spinlock_t queue_lock;
    unsigned int flags;
    bool extended_key_code;
};

static void add_keyboard_event(struct kbd_data *data, struct key_event *event) {
    (void) data;
    struct queued_umessage *umessage =
        net_create_umessage(UMESSAGE_INPUT, UMESSAGE_INPUT_KEY_EVENT, 0, sizeof(struct umessage_input_key_event), event);
    net_post_umessage(umessage);
    net_drop_umessage(umessage);
}

static struct key_code_entry map[KEYBOARD_NUM_KEYCODES] = { { KEY_NULL, '\0' },
                                                            { KEY_ESC, '\033' },
                                                            { KEY_1, '1' },
                                                            { KEY_2, '2' },
                                                            { KEY_3, '3' },
                                                            { KEY_4, '4' },
                                                            { KEY_5, '5' },
                                                            { KEY_6, '6' },
                                                            { KEY_7, '7' },
                                                            { KEY_8, '8' },
                                                            { KEY_9, '9' },
                                                            { KEY_0, '0' },
                                                            { KEY_MINUS, '-' },
                                                            { KEY_EQUALS, '=' },
                                                            { KEY_BACKSPACE, '\b' },
                                                            { KEY_TAB, '\t' },
                                                            { KEY_Q, 'q' },
                                                            { KEY_W, 'w' },
                                                            { KEY_E, 'e' },
                                                            { KEY_R, 'r' },
                                                            { KEY_T, 't' },
                                                            { KEY_Y, 'y' },
                                                            { KEY_U, 'u' },
                                                            { KEY_I, 'i' },
                                                            { KEY_O, 'o' },
                                                            { KEY_P, 'p' },
                                                            { KEY_LEFT_SQUARE_BRACKET, '[' },
                                                            { KEY_RIGHT_SQUARE_BRACKET, ']' },
                                                            { KEY_ENTER, '\r' },
                                                            { KEY_LEFT_CONTROL, '\0' },
                                                            { KEY_A, 'a' },
                                                            { KEY_S, 's' },
                                                            { KEY_D, 'd' },
                                                            { KEY_F, 'f' },
                                                            { KEY_G, 'g' },
                                                            { KEY_H, 'h' },
                                                            { KEY_J, 'j' },
                                                            { KEY_K, 'k' },
                                                            { KEY_L, 'l' },
                                                            { KEY_SEMICOLON, ';' },
                                                            { KEY_SINGLE_QUOTE, '\'' },
                                                            { KEY_BACK_TICK, '`' },
                                                            { KEY_LEFT_SHIFT, '\0' },
                                                            { KEY_BACK_SLASH, '\\' },
                                                            { KEY_Z, 'z' },
                                                            { KEY_X, 'x' },
                                                            { KEY_C, 'c' },
                                                            { KEY_V, 'v' },
                                                            { KEY_B, 'b' },
                                                            { KEY_N, 'n' },
                                                            { KEY_M, 'm' },
                                                            { KEY_COMMA, ',' },
                                                            { KEY_DOT, '.' },
                                                            { KEY_FORWARD_SLASH, '/' },
                                                            { KEY_RIGHT_SHIFT, '\0' },
                                                            { KEY_NUMPAD_STAR, '*' },
                                                            { KEY_LEFT_ALT, '\0' },
                                                            { KEY_SPACE, ' ' },
                                                            { KEY_CAPSLOCK, '\0' },
                                                            { KEY_F1, '\0' },
                                                            { KEY_F2, '\0' },
                                                            { KEY_F3, '\0' },
                                                            { KEY_F4, '\0' },
                                                            { KEY_F5, '\0' },
                                                            { KEY_F6, '\0' },
                                                            { KEY_F7, '\0' },
                                                            { KEY_F8, '\0' },
                                                            { KEY_F9, '\0' },
                                                            { KEY_F10, '\0' },
                                                            { KEY_NUMLOCK, '\0' },
                                                            { KEY_SCROLL_LOCK, '\0' },
                                                            { KEY_NUMPAD_7, '7' },
                                                            { KEY_NUMPAD_8, '8' },
                                                            { KEY_NUMPAD_9, '9' },
                                                            { KEY_NUMPAD_MINUS, '-' },
                                                            { KEY_NUMPAD_4, '4' },
                                                            { KEY_NUMPAD_5, '5' },
                                                            { KEY_NUMPAD_6, '6' },
                                                            { KEY_NUMPAD_PLUS, '+' },
                                                            { KEY_NUMPAD_1, '1' },
                                                            { KEY_NUMPAD_2, '2' },
                                                            { KEY_NUMPAD_3, '3' },
                                                            { KEY_NUMPAD_0, '0' },
                                                            { KEY_NUMPAD_DOT, '.' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_F11, '\0' },
                                                            { KEY_F12, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },

                                                            /* Extended Part */
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_PREV_TRACK, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NEXT_TRACX, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NUMPAD_ENTER, '\r' },
                                                            { KEY_RIGHT_CONTROL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_MUTE, '\0' },
                                                            { KEY_CALC, '\0' },
                                                            { KEY_PLAY, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_STOP, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_VOLUMNE_DOWN, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_VOLUME_UP, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_WWW, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NUMPAD_FORWARD_SLASH, '/' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_RIGHT_ALT, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_HOME, '\0' },
                                                            { KEY_CURSOR_UP, '\0' },
                                                            { KEY_PAGE_UP, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_CURSOR_LEFT, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_CURSOR_RIGHT, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_END, '\0' },
                                                            { KEY_CURSOR_DOWN, '\0' },
                                                            { KEY_PAGE_DOWN, '\0' },
                                                            { KEY_INSERT, '\0' },
                                                            { KEY_DELETE, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_LEFT_GUI, '\0' },
                                                            { KEY_RIGHT_GUI, '\0' },
                                                            { KEY_APPS, '\0' },
                                                            { KEY_ACPI_POWER, '\0' },
                                                            { KEY_ACPI_SLEEP, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_ACPI_WAKE, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_WWW_SEARCH, '\0' },
                                                            { KEY_WWW_FAVORITES, '\0' },
                                                            { KEY_WWW_REFRESH, '\0' },
                                                            { KEY_WWW_STOP, '\0' },
                                                            { KEY_WWW_FORWARD, '\0' },
                                                            { KEY_WWW_BACK, '\0' },
                                                            { KEY_MY_COMPUTER, '\0' },
                                                            { KEY_EMAIL, '\0' },
                                                            { KEY_MEDIA_SELECT, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' },
                                                            { KEY_NULL, '\0' } };

static struct key_code_entry shift_map[KEYBOARD_RELEASED_OFFSET] = {
    { KEY_NULL, '\0' },
    { KEY_ESC, '\033' },
    { KEY_1, '!' },
    { KEY_2, '@' },
    { KEY_3, '#' },
    { KEY_4, '$' },
    { KEY_5, '%' },
    { KEY_6, '^' },
    { KEY_7, '&' },
    { KEY_8, '*' },
    { KEY_9, '(' },
    { KEY_0, ')' },
    { KEY_MINUS, '_' },
    { KEY_EQUALS, '+' },
    { KEY_BACKSPACE, '\b' },
    { KEY_TAB, '\t' },
    { KEY_Q, 'Q' },
    { KEY_W, 'W' },
    { KEY_E, 'E' },
    { KEY_R, 'R' },
    { KEY_T, 'T' },
    { KEY_Y, 'Y' },
    { KEY_U, 'U' },
    { KEY_I, 'I' },
    { KEY_O, 'O' },
    { KEY_P, 'P' },
    { KEY_LEFT_SQUARE_BRACKET, '{' },
    { KEY_RIGHT_SQUARE_BRACKET, '}' },
    { KEY_ENTER, '\r' },
    { KEY_LEFT_CONTROL, '\0' },
    { KEY_A, 'A' },
    { KEY_S, 'S' },
    { KEY_D, 'D' },
    { KEY_F, 'F' },
    { KEY_G, 'G' },
    { KEY_H, 'H' },
    { KEY_J, 'J' },
    { KEY_K, 'K' },
    { KEY_L, 'L' },
    { KEY_SEMICOLON, ':' },
    { KEY_SINGLE_QUOTE, '"' },
    { KEY_BACK_TICK, '~' },
    { KEY_LEFT_SHIFT, '\0' },
    { KEY_BACK_SLASH, '|' },
    { KEY_Z, 'Z' },
    { KEY_X, 'X' },
    { KEY_C, 'C' },
    { KEY_V, 'V' },
    { KEY_B, 'B' },
    { KEY_N, 'N' },
    { KEY_M, 'M' },
    { KEY_COMMA, '<' },
    { KEY_DOT, '>' },
    { KEY_FORWARD_SLASH, '?' },
    { KEY_RIGHT_SHIFT, '\0' },
    { KEY_NUMPAD_STAR, '*' },
    { KEY_LEFT_ALT, '\0' },
    { KEY_SPACE, ' ' },
    { KEY_CAPSLOCK, '\0' },
    { KEY_F1, '\0' },
    { KEY_F2, '\0' },
    { KEY_F3, '\0' },
    { KEY_F4, '\0' },
    { KEY_F5, '\0' },
    { KEY_F6, '\0' },
    { KEY_F7, '\0' },
    { KEY_F8, '\0' },
    { KEY_F9, '\0' },
    { KEY_F10, '\0' },
    { KEY_NUMLOCK, '\0' },
    { KEY_SCROLL_LOCK, '\0' },
    { KEY_NUMPAD_7, '7' },
    { KEY_NUMPAD_8, '8' },
    { KEY_NUMPAD_9, '9' },
    { KEY_NUMPAD_MINUS, '-' },
    { KEY_NUMPAD_4, '4' },
    { KEY_NUMPAD_5, '5' },
    { KEY_NUMPAD_6, '6' },
    { KEY_NUMPAD_PLUS, '+' },
    { KEY_NUMPAD_1, '1' },
    { KEY_NUMPAD_2, '2' },
    { KEY_NUMPAD_3, '3' },
    { KEY_NUMPAD_0, '0' },
    { KEY_NUMPAD_DOT, '.' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_F11, '\0' },
    { KEY_F12, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
    { KEY_NULL, '\0' },
};

static void handle_keyboard_interrupt(struct irq_context *context) {
    struct device *device = context->closure;
    struct kbd_data *data = device->private;

    uint8_t scan_code;
    if (data->controller->read_byte(&scan_code)) {
        return;
    }

    if (scan_code == PS2_ACK) {
        return;
    } else if (scan_code == KEYBOARD_EXTENDED) {
        data->extended_key_code = true;
    } else if (scan_code < KEYBOARD_RELEASED_OFFSET) {
        struct key_code_entry entry;
        if (data->extended_key_code) {
            data->extended_key_code = false;
            entry = map[scan_code + KEYBOARD_RELEASED_OFFSET];
        } else {
            if (data->flags & KEY_SHIFT_ON) {
                entry = shift_map[scan_code];
            } else {
                entry = map[scan_code];
            }
        }

        if (entry.key == KEY_LEFT_CONTROL || entry.key == KEY_RIGHT_CONTROL) {
            data->flags |= KEY_CONTROL_ON;
        }

        if (entry.key == KEY_LEFT_ALT || entry.key == KEY_RIGHT_ALT) {
            data->flags |= KEY_ALT_ON;
        }

        if (entry.key == KEY_LEFT_SHIFT || entry.key == KEY_RIGHT_SHIFT) {
            data->flags |= KEY_SHIFT_ON;
        }

        data->flags |= KEY_DOWN;
        data->flags &= ~KEY_UP;

        data->event.ascii = entry.ascii;
        data->event.flags = data->flags;
        data->event.key = entry.key;

        add_keyboard_event(data, &data->event);
    } else {
        struct key_code_entry entry;
        if (data->extended_key_code) {
            data->extended_key_code = false;
            entry = map[scan_code];
        } else {
            entry = map[scan_code - KEYBOARD_RELEASED_OFFSET];
        }

        if (entry.key == KEY_LEFT_CONTROL || entry.key == KEY_RIGHT_CONTROL) {
            data->flags &= ~KEY_CONTROL_ON;
        }

        if (entry.key == KEY_LEFT_ALT || entry.key == KEY_RIGHT_ALT) {
            data->flags &= ~KEY_ALT_ON;
        }

        if (entry.key == KEY_LEFT_SHIFT || entry.key == KEY_RIGHT_SHIFT) {
            data->flags &= ~KEY_SHIFT_ON;
        }

        data->flags &= ~KEY_DOWN;
        data->flags |= KEY_UP;

        data->event.ascii = '\0';
        data->event.flags = data->flags;
        data->event.key = entry.key;

        add_keyboard_event(data, &data->event);
    }
}

static void kbd_create(struct ps2_controller *controller, struct ps2_port *port) {
    struct device *device = calloc(1, sizeof(struct device) + sizeof(struct kbd_data));
    struct kbd_data *data = device->private = device + 1;
    device->device_number = 0x00701;
    device->type = S_IFCHR;
    dev_register(device);

    init_spinlock(&data->queue_lock);
    data->controller = controller;
    data->port = port;
    data->device = device;
    data->irq_handler.closure = device;
    data->irq_handler.flags = IRQ_HANDLER_EXTERNAL;
    data->irq_handler.handler = handle_keyboard_interrupt;
    register_irq_handler(&data->irq_handler, port->irq);

    controller->send_command(PS2_DEVICE_COMMAND_ENABLE_SCANNING, port->port_number);
}

static struct ps2_device_id kbd_ids[] = {
    { 0x41, 0x00 }, { 0x83, 0x00 }, { 0xAB, 0x41 }, { 0xAB, 0xC1 }, { 0xAB, 0x83 },
};

static struct ps2_driver kbd_driver = {
    .name = "PS/2 Keyboard",
    .device_id_list = kbd_ids,
    .device_id_count = sizeof(kbd_ids) / sizeof(struct ps2_device_id),
    .create = kbd_create,
};

void init_keyboard() {
    ps2_register_driver(&kbd_driver);
}
