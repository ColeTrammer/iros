#ifndef _KERNEL_HAL_X86_64_DRIVERS_MOUSE_H
#define _KERNEL_HAL_X86_64_DRIVERS_MOUSE_H 1

#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>

#define PS2_CONTROL_REGISTER 0x64
#define PS2_DATA_REGISTER    0x60

#define MOUSE_ACK 0xFA

#define MOUSE_COMMAND_RESET           0xFF
#define MOUSE_COMMAND_RESENT          0xFE
#define MOUSE_COMMAND_SET_DEFAULTS    0xF6
#define MOUSE_COMMAND_DISABLE_PACKETS 0xF5
#define MOUSE_COMMAND_ENABLE_PACKETS  0xF4
#define MOUSE_COMMAND_SET_SAMPLE_RATE 0xF3
#define MOUSE_COMMAND_ID              0xF2
#define MOUSE_COMMAND_SINGLE_PACKET   0xEB
#define MOUSE_COMMAND_SET_COMPAQ      0x60
#define MOUSE_COMMAND_GET_COMPAQ      0x20

#define MOUSE_IRQ_LINE_NUM 12

static inline void mouse_wait_for_output() {
    while (inb(PS2_CONTROL_REGISTER) & 0x2)
        ;
}

static inline void mouse_wait_for_input() {
    while (!(inb(PS2_CONTROL_REGISTER) & 0x1))
        ;
}

static inline void mouse_send_command(uint8_t byte) {
    mouse_wait_for_output();
    outb(PS2_CONTROL_REGISTER, 0xD4);
    mouse_wait_for_output();
    outb(PS2_DATA_REGISTER, byte);
}

static inline uint8_t mouse_read() {
    mouse_wait_for_input();
    return inb(PS2_DATA_REGISTER);
}

void init_mouse();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_MOUSE_H */
