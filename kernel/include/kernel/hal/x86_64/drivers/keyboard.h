#ifndef _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H
#define _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H 1

#include <kernel/hal/input.h>

#define KEYBOARD_SET_LEDS          0xED
#define KEYBOARD_ECHO              0xEE
#define KEYBOARD_SET_SCAN_CODE_SET 0xF0
#define KEYBOARD_GET_ID            0xF2
#define KEYBOARD_TYPEMATIC         0xF3
#define KEYBOARD_ENABLE            0xF4
#define KEYBOARD_DISABLE           0xF5
#define KEYBOARD_RESET             0xF6

#define KEYBOARD_SCAN_CODE_SET 2 /* Is actually 1 but QEMU disagrees */

#define KEYBOARD_EXTENDED        0xE0
#define KEYBOARD_RELEASED_OFFSET 0x80
#define KEYBOARD_NUM_KEYCODES    (2 * KEYBOARD_RELEASED_OFFSET)

struct key_code_entry {
    enum key key;
    char ascii;
};

struct keyboard_event_queue {
    struct key_event entry;
    struct keyboard_event_queue *next;
};

#endif /* _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H */
