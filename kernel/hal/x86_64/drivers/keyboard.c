#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/output.h>
#include <kernel/arch/x86_64/asm_utils.h>

volatile uint8_t *kbd_buffer;

static size_t kbd_index = 0;

static uint8_t char_map[] = {
    '\0',
    '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\0', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u',
    'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f',
    'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '*',
    '\0', ' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0'
};

struct keyboard_task {
    uint8_t command;
    struct keyboard_task *next;
};

static struct keyboard_task *first = NULL;
static struct keyboard_task *last = NULL;

static bool extended_key_code = false;

static bool pressed[KEYBOARD_NUM_KEYCODES];

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
    debug_log("Keyboard Interrupt: [ %#.2X ]\n", scan_code);

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
        if (extended_key_code) {
            pressed[scan_code + KEYBOARD_RELEASED_OFFSET] = true;
            extended_key_code = false;
        } else {
            pressed[scan_code] = true;

            if (char_map[scan_code] != '\0') {
                kbd_buffer[kbd_index++] = char_map[scan_code];
            }
        }
    } else {
        if (extended_key_code) {
            pressed[scan_code] = false;
            extended_key_code = false;
        } else {
            pressed[scan_code - KEYBOARD_RELEASED_OFFSET] = false;
        }
    }
}

void init_keyboard() {
    kbd_buffer = calloc(1, 0x1000);

    register_irq_line_handler(&handle_keyboard_interrupt, KEYBOARD_IRQ_LINE, true);

    add_keyboard_task(create_keyboard_task(KEYBOARD_SET_SCAN_CODE_SET));
    add_keyboard_task(create_keyboard_task(KEYBOARD_SCAN_CODE_SET));

    exec_keyboard_task();
}