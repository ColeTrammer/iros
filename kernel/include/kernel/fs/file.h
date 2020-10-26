#ifndef _KERNEL_FS_FILE_H
#define _KERNEL_FS_FILE_H 1

#include <poll.h>
#include <stdint.h>
#include <sys/types.h>

#include <kernel/proc/wait_queue.h>
#include <kernel/util/mutex.h>

struct file;
struct inode;
struct tnode;

struct file_state {
    int poll_flags;
    struct wait_queue queue;
};

static inline void init_file_state(struct file_state *state, bool readable, bool writeable) {
    state->poll_flags = (readable ? POLLIN : 0) | (writeable ? POLLOUT : 0);
    init_wait_queue(&state->queue);
}

static inline void fs_set_state(struct file_state *state, int flags) {
    if (state->poll_flags == flags) {
        return;
    }

    state->poll_flags = flags;
    wake_up_all(&state->queue);
}

static inline void fs_trigger_state(struct file_state *state, int flags) {
    fs_set_state(state, state->poll_flags | flags);
}

static inline void fs_detrigger_state(struct file_state *state, int flags) {
    fs_set_state(state, state->poll_flags & ~flags);
}

static inline void fs_set_state_bit(struct file_state *state, int bit, bool val) {
    if (val) {
        fs_trigger_state(state, bit);
    } else {
        fs_detrigger_state(state, bit);
    }
}

struct file_operations {
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, off_t offset, void *buffer, size_t len);
    ssize_t (*write)(struct file *file, off_t offset, const void *buffer, size_t len);
    int (*poll)(struct file *file, struct wait_queue_entry *entry, int mask);
    void (*poll_finish)(struct file *file, struct wait_queue_entry *entry);
};

struct file {
    off_t position;

    struct file_operations *f_op;
    unsigned int flags;

    int open_flags;

#define FS_FILE_CAN_READ  1
#define FS_FILE_CAN_WRITE 2
#define FS_FILE_CANT_SEEK 4
    int abilities;

    int ref_count;
    mutex_t lock;

    struct tnode *tnode;
    struct inode *inode;

    void *private_data;
};

#endif /* _KERNEL_FS_FILE_H */
