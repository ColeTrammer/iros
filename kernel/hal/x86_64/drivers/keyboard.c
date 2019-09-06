#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/arch/x86_64/asm_utils.h>

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
    register_irq_line_handler(&handle_keyboard_interrupt, KEYBOARD_IRQ_LINE, true);

    add_keyboard_task(create_keyboard_task(KEYBOARD_SET_SCAN_CODE_SET));
    add_keyboard_task(create_keyboard_task(KEYBOARD_SCAN_CODE_SET));

    exec_keyboard_task();
}