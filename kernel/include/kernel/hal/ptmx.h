#ifndef _KERNEL_HAL_PTMX_H
#define _KERNEL_HAL_PTMX_H 1

#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>

#include <kernel/util/spinlock.h>

#include <kernel/hal/arch.h>
#include HAL_ARCH_SPECIFIC(drivers/vga.h)

#define TTY_BUF_MAX_START 256

struct tty_buffer {
    char *buffer;
    size_t buffer_length;
    size_t buffer_max;
};

struct slave_data {
    spinlock_t lock;
    int ref_count;
    int index;

    struct termios config;
    struct tty_buffer buffer;
};

struct master_data {
    int index;

    struct tty_buffer buffer;
};

void init_ptmx();

#endif /* _KERNEL_HAL_PTMX_H */