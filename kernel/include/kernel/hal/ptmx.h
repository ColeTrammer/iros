#ifndef _KERNEL_HAL_PTMX_H
#define _KERNEL_HAL_PTMX_H 1

#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>

#include <kernel/util/ring_buffer.h>
#include <kernel/util/spinlock.h>

#define PTMX_BUFFER_MAX 4096

struct slave_data {
    int ref_count;
    int index;

    unsigned short rows;
    unsigned short cols;

    // FIXME: we should respect these fields
    bool output_enabled : 1;
    bool input_enabled : 1;

    pid_t pgid;
    pid_t sid;

    struct ring_buffer input_buffer;

    struct termios config;
    struct fs_device *device;
};

struct master_data {
    int index;

    struct ring_buffer output_buffer;

    char *input_buffer;
    size_t input_buffer_length;
    size_t input_buffer_max;

    struct fs_device *device;
    struct tty_buffer_message *messages;
};

void init_ptmx();

#endif /* _KERNEL_HAL_PTMX_H */
