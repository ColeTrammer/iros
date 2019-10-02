#ifndef _KERNEL_HAL_TTY_H
#define _KERNEL_HAL_TTY_H 1

#include <sys/types.h>
#include <termios.h>

#include <kernel/hal/input.h>
#include <kernel/fs/file.h>
#include <kernel/util/spinlock.h>

#define DEFAULT_TTY_WIDTH 80UL
#define DEFUALT_TTY_HEIGHT 25UL

struct tty_data {
    size_t x;
    size_t y;
    size_t x_max;
    size_t y_max;
    unsigned char *buffer;
    struct file *keyboard;
    struct key_event key_buffer;
    char *input_buffer;
    size_t input_buffer_length;
    size_t input_buffer_offset;
    struct termios config;
    spinlock_t lock;
};

void init_tty_device(dev_t dev);

#endif /* _KERNEL_HAL_TTY_H */