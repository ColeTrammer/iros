#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/output.h>
#include <kernel/hal/input.h>
#include <kernel/fs/dev.h>
#include <kernel/sched/process_sched.h>
#include <kernel/util/spinlock.h>
#include <kernel/arch/x86_64/asm_utils.h>

static struct keyboard_event_queue *start;
static struct keyboard_event_queue *end;
static spinlock_t queue_lock = SPINLOCK_INITIALIZER;

static ssize_t kbd_read(struct device *device, void *buffer, size_t len) {
    (void) device;

    ssize_t read = 0;
    
    while (len >= sizeof(struct key_event)) {
        do {
            while (start == NULL) {
                barrier();
            }
        } while (start == NULL);

        while (len >= sizeof(struct key_event) && start != NULL) {
            assert(start);
            assert(&start->entry);
            assert(buffer);
            memcpy(((uint8_t*) buffer) + read, &start->entry, sizeof(struct key_event));

            len -= sizeof(struct key_event);
            read += sizeof(struct key_event);

            if (start->next == NULL) {
                end = NULL;
            }

            struct keyboard_event_queue *save = start->next;
            // free(start);
            start = save;
        }

        spin_unlock(&queue_lock);
    }

    return read;
}

static struct device_ops kbd_ops = {
    NULL, kbd_read, NULL, NULL, NULL, NULL
};

static void add_keyboard_event(struct key_event *event) {
    struct keyboard_event_queue *e = malloc(sizeof(struct keyboard_event_queue));
    assert(e);
    assert(event);
    assert(&e->entry);
    memcpy(&e->entry, event, sizeof(struct key_event));

    spin_lock(&queue_lock);

    if (start == NULL) {
        start = end = e;
        e->next = NULL;
    } else {
        end->next = e;
        end = e;
    }

    spin_unlock(&queue_lock);
}

static struct key_code_entry map[KEYBOARD_NUM_KEYCODES] = {
    { KEY_NULL, '\0' },
    { KEY_ESC, '\0' },
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
    { KEY_BACKSPACE, '\0' },
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
    { KEY_ENTER, '\n' },
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

    /* Extended Part */
    { KEY_NULL, '\0' },
    { KEY_ESC, '\0' },
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
    { KEY_BACKSPACE, '\0' },
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
    { KEY_ENTER, '\n' },
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
    { KEY_NULL, '\0' }
};

struct keyboard_task {
    uint8_t command;
    struct keyboard_task *next;
};

static struct keyboard_task *first = NULL;
static struct keyboard_task *last = NULL;

static bool extended_key_code = false;
static int flags = 0;

static struct key_event event;

static struct keyboard_task *create_keyboard_task(uint8_t command) {
    struct keyboard_task *task = malloc(sizeof(struct keyboard_task));
    task->command = command;
    return task;
}

static void add_keyboard_task(struct keyboard_task *task) {
    if (first == NULL) {
        first = last = task;
    }
    last->next = task;
    last = task;
}

static void exec_keyboard_task() {
    outb(KEYBOARD_DATA_PORT, first->command);
}

static void handle_keyboard_interrupt() {
    uint8_t scan_code = inb(KEYBOARD_DATA_PORT);

    if (scan_code == KEYBOARD_ACK) {
        void *temp = first->next;
        free(first);
        first = temp;
        if (first != NULL) {
            exec_keyboard_task();
        }
    } else if (scan_code == KEYBOARD_EXTENDED) {
        extended_key_code = true;
    } else if (scan_code < KEYBOARD_RELEASED_OFFSET) {
        struct key_code_entry entry;
        if (extended_key_code) {
            extended_key_code = false;
            entry = map[scan_code + KEYBOARD_RELEASED_OFFSET];
        } else {
            entry = map[scan_code];
        }

        if (entry.key == KEY_LEFT_CONTROL || entry.key == KEY_RIGHT_CONTROL) {
            flags |= KEY_CONTROL_ON;
        }

        if (entry.key == KEY_LEFT_ALT || entry.key == KEY_RIGHT_ALT) {
            flags |= KEY_ALT_ON;
        }

        if (entry.key == KEY_LEFT_SHIFT || entry.key == KEY_RIGHT_SHIFT) {
            flags |= KEY_SHIFT_ON;
        }

        flags |= KEY_DOWN;
        flags &= KEY_UP;

        event.ascii = entry.ascii;
        event.flags = flags;
        event.key = entry.key;

        add_keyboard_event(&event);
    } else {
        struct key_code_entry entry;
        if (extended_key_code) {
            extended_key_code = false;
            entry = map[scan_code];
        } else {
            entry = map[scan_code - KEYBOARD_RELEASED_OFFSET];
        }

        if (entry.key == KEY_LEFT_CONTROL || entry.key == KEY_RIGHT_CONTROL) {
            flags |= KEY_CONTROL_ON;
        }

        if (entry.key == KEY_LEFT_ALT || entry.key == KEY_RIGHT_ALT) {
            flags |= KEY_ALT_ON;
        }

        if (entry.key == KEY_LEFT_SHIFT || entry.key == KEY_RIGHT_SHIFT) {
            flags |= KEY_SHIFT_ON;
        }

        flags &= KEY_DOWN;
        flags |= KEY_UP;

        event.ascii = '\0';
        event.flags = flags;
        event.key = entry.key;

        add_keyboard_event(&event);
    }
}

void init_keyboard() {
    register_irq_line_handler(&handle_keyboard_interrupt, KEYBOARD_IRQ_LINE, true);

    add_keyboard_task(create_keyboard_task(KEYBOARD_SET_SCAN_CODE_SET));
    add_keyboard_task(create_keyboard_task(KEYBOARD_SCAN_CODE_SET));

    struct device *device = malloc(sizeof(struct device));
    device->device_number = 0x20;
    strcpy(device->name, "keyboard");
    device->ops = &kbd_ops;
    device->private = NULL;
    dev_add(device, device->name);

    exec_keyboard_task();
}