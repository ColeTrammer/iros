#ifndef _KERNEL_HAL_X86_DRIVERS_VMWARE_BACK_DOOR_H
#define _KERNEL_HAL_X86_DRIVERS_VMWARE_BACK_DOOR_H 1

#include <stdbool.h>
#include <stdint.h>

#define VMWARE_MAGIC   0x564D5868
#define VMWARE_IO_PORT 0x5658

// Commands from table at https://sites.google.com/site/chitchatvmback/backdoor
#define VMWARE_GET_PROCESSOR_SPEED0x1
#define VMWARE_APM_FUNCTION              0x02
#define VMWARE_GET_MOUSE_POSITION        0x04
#define VMWARE_SET_MOUSE_POSITION        0x05
#define VMWARE_GET_CLIPBOARD_TEXT_LENGTH 0x06
#define VMWARE_GET_CLIPBOARD_TEXT        0x07
#define VMWARE_SET_CLIPBOARD_TEXT_LENGTH 0x08
#define VMWARE_SET_CLIPBOARD_TEXT        0x09
#define VMWARE_GET_VERSION               0x0A
#define VMWARE_GET_DEVICE_INFO           0x0B
#define VMWARE_CONNECT_DEVICE            0x0C
#define VMWARE_GET_GUI_SETTINGS          0x0D
#define VMWARE_SET_GUI_SETTINGS          0x0E
#define VMWARE_GET_HOST_SCREEN_SIZE      0x0F
#define VMWARE_GET_HARDWARE_VERSION      0x11
#define VMWARE_POPUP_DIALOG              0x12
#define VMWARE_GET_BIOS_UUID             0x13
#define VMWARE_GET_MEMORY_SIZE           0x14
#define VMWARE_GET_SYSTEM_TIME           0x17

// VM Mouse Commands
#define VMWARE_VMMOUSE_DATA    0x27
#define VMWARE_VMMOUSE_STATUS  0x28
#define VMWARE_VMMOUSE_COMMAND 0x29

#define VMMOUSE_GET_ID            0x45414552
#define VMMOUSE_DISABLE           0x000000F5
#define VMMOUSE_RELATIVE_POSITION 0x4C455252
#define VMMOUSE_ABSOLUTE_POSITION 0x53424152

#define VMMOUSE_QEMU_VERSION 0x3442554a

struct mouse_event *vmmouse_read(void);
bool vmmouse_is_enabled(void);

bool vmware_back_door_is_enabled(void);

static inline __attribute__((always_inline)) uint32_t vmware_send(uint32_t command, uint32_t arg) {
    uint32_t ret;
    asm volatile("in %%dx, %0" : "=a"(ret) : "a"(VMWARE_MAGIC), "b"(arg), "c"(command), "d"(VMWARE_IO_PORT));
    return ret;
}

struct vmware_registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};

static inline __attribute__((always_inline)) struct vmware_registers vmware_send_full(uint32_t command, uint32_t arg) {
    struct vmware_registers regs;
    asm volatile("in %%dx, %0"
                 : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
                 : "a"(VMWARE_MAGIC), "b"(arg), "c"(command), "d"(VMWARE_IO_PORT));
    return regs;
}

#endif /* _KERNEL_HAL_X86_DRIVERS_VMWARE_BACK_DOOR_H */
