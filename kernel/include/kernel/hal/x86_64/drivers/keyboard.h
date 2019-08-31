#ifndef _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H
#define _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H 1

#define KEYBOARD_IRQ_LINE 1

#define KEYBOARD_DATA_PORT 0x60

#define KEYBOARD_SET_LEDS 0xED
#define KEYBOARD_ECHO 0xEE
#define KEYBOARD_SET_SCAN_CODE_SET 0xF0
#define KEYBOARD_GET_ID 0xF2
#define KEYBOARD_TYPEMATIC 0xF3
#define KEYBOARD_ENABLE 0xF4
#define KEYBOARD_DISABLE 0xF5
#define KEYBOARD_RESET 0xF6

#define KEYBOARD_ACK 0xFA
#define KEYBOARD_RESEND 0xFE

#define KEYBOARD_SCAN_CODE_SET 2 /* Is actually 1 but QEMU disagrees */

#define KEYBOARD_EXTENDED 0xE0
#define KEYBOARD_RELEASED_OFFSET 0x80
#define KEYBOARD_NUM_KEYCODES (2 * KEYBOARD_RELEASED_OFFSET)

void init_keyboard();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_KEYBOARD_H */