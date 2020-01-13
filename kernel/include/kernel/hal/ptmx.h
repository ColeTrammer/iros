#ifndef _KERNEL_HAL_PTMX_H
#define _KERNEL_HAL_PTMX_H 1

#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>

#include <kernel/util/spinlock.h>

// clang-format off
#include <kernel/hal/arch.h>
#include HAL_ARCH_SPECIFIC(drivers/vga.h)
// clang-format on

#define TTY_BUF_MAX_START 256

struct tty_buffer_message {
    struct tty_buffer_message *next;
    struct tty_buffer_message *prev;
    size_t len;
    size_t max;
    char *buf;
};

struct slave_data {
    spinlock_t lock;
    int ref_count;
    int index;

    unsigned short rows;
    unsigned short cols;

    // FIXME: we should respect these fields
    bool output_enabled : 1;
    bool input_enabled : 1;

    pid_t pgid;
    pid_t sid;

    char *input_buffer;
    size_t input_buffer_index;
    size_t input_buffer_length;
    size_t input_buffer_max;

    struct termios config;
    struct device *device;
    struct tty_buffer_message *messages;
};

struct master_data {
    spinlock_t lock;
    int index;

    char *output_buffer;
    size_t output_buffer_index;
    size_t output_buffer_length;
    size_t output_buffer_max;

    char *input_buffer;
    size_t input_buffer_length;
    size_t input_buffer_max;

    struct device *device;
    struct tty_buffer_message *messages;
};

void init_ptmx();

#endif /* _KERNEL_HAL_PTMX_H */