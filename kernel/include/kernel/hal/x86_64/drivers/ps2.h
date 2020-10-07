#ifndef _KERNEL_HAL_X86_64_DRIVERS_PS2_H
#define _KERNEL_HAL_X86_64_DRIVERS_PS2_H 1

#include <stdint.h>

#include <kernel/hal/hw_device.h>
#include <kernel/util/list.h>

#define PS2_IO_DATA    0x60
#define PS2_IO_STATUS  0x64 // Reads
#define PS2_IO_COMMAND 0x64 // Writes

#define PS2_IRQ0 1
#define PS2_IRQ1 12

#define PS2_COMMAND_READ_CONFIG       0x20
#define PS2_COMMAND_WRITE_CONFIG      0x60
#define PS2_COMMAND_DISABLE_PORT1     0xA7
#define PS2_COMMAND_ENABLE_PORT1      0xA8
#define PS2_COMMAND_TEST_PORT1        0xA9
#define PS2_COMMAND_TEST_CONTROLLER   0xAA
#define PS2_COMMAND_TEST_PORT0        0xAB
#define PS2_COMMAND_DISABLE_PORT0     0xAD
#define PS2_COMMAND_ENABLE_PORT0      0xAE
#define PS2_COMMAND_READ_OUTPUT_PORT  0xD0
#define PS2_COMMAND_WRITE_OUTPUT_PORT 0xD1
#define PS2_COMMAND_WRITE_PORT1       0xD4

#define PS2_STATUS_OUTPUT_FULL   (1U << 0U)
#define PS2_STATUS_INPUT_FULL    (1U << 1U)
#define PS2_STATUS_SYSTEM        (1U << 2U)
#define PS2_STATUS_COMMAND       (1U << 3U)
#define PS2_STATUS_TIMEOUT_ERROR (1U << 6U)
#define PS2_STATUS_PARITY_ERROR  (1U << 7U)

#define PS2_CONFIG_IRQ0_ENABLED    (1U << 0U)
#define PS2_CONFIG_IRQ1_ENABLED    (1U << 1U)
#define PS2_CONFIG_SYSTEM          (1U << 2U)
#define PS2_CONFIG_CLOCK0_DISABLED (1U << 4U)
#define PS2_CONFIG_CLOCK1_DISABLED (1U << 5U)
#define PS2_CONFIG_TRANLATE        (1U << 6U)

#define PS2_OUTPUT_SYSTEM_RESET    (1U << 0U)
#define PS2_OUTPUT_A20             (1U << 1U)
#define PS2_OUTPUT_CLOCK1          (1U << 2U)
#define PS2_OUTPUT_DATA1           (1U << 3U)
#define PS2_OUTPUT_DATA_FROM_PORT0 (1U << 4U)
#define PS2_OUTPUT_DATA_FROM_PORT1 (1U << 5U)
#define PS2_OUTPUT_CLOCK0          (1U << 6U)
#define PS2_OUTPUT_DATA0           (1U << 7U)

#define PS2_DEVICE_COMMAND_ID               0xF2
#define PS2_DEVICE_COMMAND_SET_SAMPLE_RATE  0xF3
#define PS2_DEVICE_COMMAND_ENABLE_SCANNING  0xF4
#define PS2_DEVICE_COMMAND_DISABLE_SCANNING 0xF5
#define PS2_DEVICE_COMMAND_SET_DEFAULTS     0xF6
#define PS2_DEVICE_COMMAND_SELF_TEST        0xFF

#define PS2_CONTROLLER_SELF_TEST_SUCCESS 0x55
#define PS2_DEVICE_SELF_TEST_SUCCESS     0xAA
#define PS2_ACK                          0xFA
#define PS2_RESEND                       0xFE

struct fs_device;

struct ps2_port {
    struct ps2_device_id id;
    int irq;
    int port_number;
};

struct ps2_controller {
    int (*send_byte)(uint8_t byte, int port_number);
    int (*send_command)(uint8_t command, int port_number);
    int (*read_byte)(uint8_t *bytep);
    struct ps2_port ports[2];
};

struct ps2_driver {
    const char *name;
    struct ps2_device_id *device_id_list;
    size_t device_id_count;
    void (*create)(struct ps2_controller *controller, struct ps2_port *port);
    struct list_node list;
};

void ps2_register_driver(struct ps2_driver *driver);
void ps2_unregister_driver(struct ps2_driver *driver);

void init_ps2_controller(void);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_PS2_H */
