#include <assert.h>
#include <limits.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/pipe.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/net/socket.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>

static bool until_inode_is_readable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_INODE_IS_READABLE);
    return info->until_inode_is_readable_info.inode->readable || info->until_inode_is_readable_info.inode->excetional_activity;
}

int proc_block_until_inode_is_readable(struct task *current, struct inode *inode) {
    current->block_info.until_inode_is_readable_info.inode = inode;
    current->block_info.type = UNTIL_INODE_IS_READABLE;
    current->block_info.should_unblock = &until_inode_is_readable_blocker;
    return wait_with_blocker(current);
}

static bool until_socket_is_connected_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_CONNECTED);

    return info->until_socket_is_connected_info.socket->state >= CONNECTED;
}

int proc_block_until_socket_is_connected(struct task *current, struct socket *socket) {
    assert(socket->state != CONNECTED);
    current->block_info.until_socket_is_connected_info.socket = socket;
    current->block_info.type = UNTIL_SOCKET_IS_CONNECTED;
    current->block_info.should_unblock = &until_socket_is_connected_blocker;
    return wait_with_blocker(current);
}

static bool until_inode_is_readable_or_timeout_blocker(struct block_info *info) {
    assert(info->type == UNTIL_INODE_IS_READABLE_OR_TIMEOUT);

    return time_compare(time_read_clock(CLOCK_MONOTONIC), info->until_inode_is_readable_or_timeout_info.end_time) >= 0 ||
           info->until_inode_is_readable_or_timeout_info.inode->readable || info->until_inode_is_readable_info.inode->excetional_activity;
}

int proc_block_until_inode_is_readable_or_timeout(struct task *current, struct inode *inode, struct timespec end_time) {
    current->block_info.until_inode_is_readable_or_timeout_info.inode = inode;
    current->block_info.until_inode_is_readable_or_timeout_info.end_time = end_time;
    current->block_info.type = UNTIL_INODE_IS_READABLE_OR_TIMEOUT;
    current->block_info.should_unblock = &until_inode_is_readable_or_timeout_blocker;
    return wait_with_blocker(current);
}

static bool until_inode_is_writable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_INODE_IS_WRITABLE);
    return info->until_inode_is_writable_info.inode->writeable;
}

int proc_block_until_inode_is_writable(struct task *current, struct inode *inode) {
    current->block_info.until_inode_is_writable_info.inode = inode;
    current->block_info.type = UNTIL_INODE_IS_WRITABLE;
    current->block_info.should_unblock = &until_inode_is_writable_blocker;
    return wait_with_blocker(current);
}

static bool until_socket_is_readable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_READABLE);

    return info->until_socket_is_readable_info.socket->readable || info->until_socket_is_readable_info.socket->state == CLOSED;
}

int proc_block_until_socket_is_readable(struct task *current, struct socket *socket) {
    current->block_info.until_socket_is_readable_info.socket = socket;
    current->block_info.type = UNTIL_SOCKET_IS_READABLE;
    current->block_info.should_unblock = &until_socket_is_readable_blocker;
    return wait_with_blocker(current);
}

static bool until_socket_is_readable_with_timeout_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_READABLE_WITH_TIMEOUT);

    return time_compare(time_read_clock(CLOCK_MONOTONIC), info->until_socket_is_readable_with_timeout_info.end_time) >= 0 ||
           info->until_socket_is_readable_with_timeout_info.socket->readable ||
           info->until_socket_is_readable_with_timeout_info.socket->state == CLOSED;
}

int proc_block_until_socket_is_readable_with_timeout(struct task *current, struct socket *socket, struct timespec end_time) {
    current->block_info.until_socket_is_readable_with_timeout_info.socket = socket;
    current->block_info.until_socket_is_readable_with_timeout_info.end_time = end_time;
    current->block_info.type = UNTIL_SOCKET_IS_READABLE_WITH_TIMEOUT;
    current->block_info.should_unblock = &until_socket_is_readable_with_timeout_blocker;
    return wait_with_blocker(current);
}

static bool until_socket_is_writable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_WRITABLE);

    return info->until_socket_is_writable_info.socket->writable || info->until_socket_is_writable_info.socket->state == CLOSED;
}

int proc_block_until_socket_is_writable(struct task *current, struct socket *socket) {
    current->block_info.until_socket_is_writable_info.socket = socket;
    current->block_info.type = UNTIL_SOCKET_IS_WRITABLE;
    current->block_info.should_unblock = &until_socket_is_writable_blocker;
    return wait_with_blocker(current);
}

static bool until_socket_is_writable_with_timeout_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_WRITABLE_WITH_TIMEOUT);

    return time_compare(time_read_clock(CLOCK_MONOTONIC), info->until_socket_is_writable_with_timeout_info.end_time) >= 0 ||
           info->until_socket_is_writable_with_timeout_info.socket->writable ||
           info->until_socket_is_writable_with_timeout_info.socket->state == CLOSED;
}

int proc_block_until_socket_is_writable_with_timeout(struct task *current, struct socket *socket, struct timespec end_time) {
    current->block_info.until_socket_is_writable_with_timeout_info.socket = socket;
    current->block_info.until_socket_is_writable_with_timeout_info.end_time = end_time;
    current->block_info.type = UNTIL_SOCKET_IS_WRITABLE_WITH_TIMEOUT;
    current->block_info.should_unblock = &until_socket_is_writable_with_timeout_blocker;
    return wait_with_blocker(current);
}

static bool until_device_is_readable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_DEVICE_IS_READABLE);

    return info->until_device_is_readable_info.device->readable;
}

int proc_block_until_device_is_readable(struct task *current, struct fs_device *device) {
    current->block_info.until_device_is_readable_info.device = device;
    current->block_info.type = UNTIL_DEVICE_IS_READABLE;
    current->block_info.should_unblock = &until_device_is_readable_blocker;
    return wait_with_blocker(current);
}

static bool until_device_is_writeable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_DEVICE_IS_WRITEABLE);

    return info->until_device_is_writeable_info.device->writeable;
}

int proc_block_until_device_is_writeable(struct task *current, struct fs_device *device) {
    current->block_info.until_device_is_writeable_info.device = device;
    current->block_info.type = UNTIL_DEVICE_IS_WRITEABLE;
    current->block_info.should_unblock = &until_device_is_writeable_blocker;
    return wait_with_blocker(current);
}

static bool until_device_is_readable_or_timeout_blocker(struct block_info *info) {
    assert(info->type == UNTIL_DEVICE_IS_READABLE_OR_TIMEOUT);

    return time_compare(time_read_clock(CLOCK_MONOTONIC), info->until_device_is_readable_or_timeout_info.end_time) >= 0 ||
           info->until_device_is_readable_or_timeout_info.device->readable;
}

int proc_block_until_device_is_readable_or_timeout(struct task *current, struct fs_device *device, struct timespec end_time) {
    current->block_info.until_device_is_readable_or_timeout_info.device = device;
    current->block_info.until_device_is_readable_or_timeout_info.end_time = end_time;
    current->block_info.type = UNTIL_DEVICE_IS_READABLE_OR_TIMEOUT;
    current->block_info.should_unblock = &until_device_is_readable_or_timeout_blocker;
    return wait_with_blocker(current);
}

static bool until_pipe_has_readers_helper(struct block_info *info) {
    assert(info->type == UNTIL_PIPE_HAS_READERS);
    return is_pipe_read_end_open(info->until_pipe_has_readers_info.pipe_data);
}

int proc_block_until_pipe_has_readers(struct task *current, struct pipe_data *pipe_data) {
    current->block_info.until_pipe_has_readers_info.pipe_data = pipe_data;
    current->block_info.type = UNTIL_PIPE_HAS_READERS;
    current->block_info.should_unblock = &until_pipe_has_readers_helper;
    return wait_with_blocker(current);
}

static bool until_pipe_has_writers_helper(struct block_info *info) {
    assert(info->type == UNTIL_PIPE_HAS_WRITERS);
    return is_pipe_write_end_open(info->until_pipe_has_writers_info.pipe_data);
}

int proc_block_until_pipe_has_writers(struct task *current, struct pipe_data *pipe_data) {
    current->block_info.until_pipe_has_writers_info.pipe_data = pipe_data;
    current->block_info.type = UNTIL_PIPE_HAS_WRITERS;
    current->block_info.should_unblock = &until_pipe_has_writers_helper;
    return wait_with_blocker(current);
}

static bool select_blocker_helper(int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds) {
    struct task *current = get_current_task();
    size_t fd_set_size = ((nfds + sizeof(uint8_t) * CHAR_BIT - 1) / sizeof(uint8_t) / CHAR_BIT) * sizeof(uint8_t);

    if (readfds) {
        for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
            if (readfds[i]) {
                for (size_t j = 0; i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                    if (readfds[i] & (1U << j)) {
                        struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                        if (!to_check || fs_is_readable(to_check)) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (writefds) {
        for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
            if (writefds[i]) {
                for (size_t j = 0; i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                    if (writefds[i] & (1U << j)) {
                        struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                        if (!to_check || fs_is_writable(to_check)) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (exceptfds) {
        for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
            if (exceptfds[i]) {
                for (size_t j = 0; i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                    if (exceptfds[i] & (1U << j)) {
                        struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                        if (!to_check || fs_is_exceptional(to_check)) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

static bool select_blocker(struct block_info *info) {
    assert(info->type == SELECT);

    return select_blocker_helper(info->select_info.nfds, info->select_info.readfds, info->select_info.writefds,
                                 info->select_info.exceptfds);
}

int proc_block_select(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds) {
    current->block_info.select_info.nfds = nfds;
    current->block_info.select_info.readfds = readfds;
    current->block_info.select_info.writefds = writefds;
    current->block_info.select_info.exceptfds = exceptfds;
    current->block_info.type = SELECT;
    current->block_info.should_unblock = &select_blocker;
    return wait_with_blocker(current);
}

static bool select_timeout_blocker(struct block_info *info) {
    assert(info->type == SELECT_TIMEOUT);

    struct timespec now = time_read_clock(CLOCK_MONOTONIC);
    if (time_compare(now, info->select_timeout_info.end_time) >= 0) {
        return true;
    }

    return select_blocker_helper(info->select_timeout_info.nfds, info->select_timeout_info.readfds, info->select_timeout_info.writefds,
                                 info->select_timeout_info.exceptfds);
}

int proc_block_select_timeout(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds,
                              struct timespec end_time) {
    current->block_info.select_timeout_info.nfds = nfds;
    current->block_info.select_timeout_info.readfds = readfds;
    current->block_info.select_timeout_info.writefds = writefds;
    current->block_info.select_timeout_info.exceptfds = exceptfds;
    current->block_info.select_timeout_info.end_time = end_time;
    current->block_info.type = SELECT_TIMEOUT;
    current->block_info.should_unblock = &select_timeout_blocker;
    return wait_with_blocker(current);
}

static bool poll_blocker_helper(nfds_t nfds, struct pollfd *fds) {
    struct task *current = get_current_task();
    for (nfds_t i = 0; i < nfds; i++) {
        struct pollfd *pfd = &fds[i];
        int current_fd = pfd->fd;
        if (current_fd < 0) {
            continue;
        }

        struct file *file = current->process->files[current_fd].file;
        if (!file) {
            return true;
        }

        if (!!(pfd->events & POLLIN) && fs_is_readable(file)) {
            return true;
        }
        if (!!(pfd->events & POLLPRI) && fs_is_exceptional(file)) {
            return true;
        }
        if (!!(pfd->events & POLLOUT) && fs_is_writable(file)) {
            return true;
        }
    }

    return false;
}

static bool poll_blocker(struct block_info *info) {
    assert(info->type == POLL);
    return poll_blocker_helper(info->poll_info.nfds, info->poll_info.fds);
}

int proc_block_poll(struct task *current, nfds_t nfds, struct pollfd *fds) {
    current->block_info.poll_info.nfds = nfds;
    current->block_info.poll_info.fds = fds;
    current->block_info.type = POLL;
    current->block_info.should_unblock = &poll_blocker;
    return wait_with_blocker(current);
}

static bool poll_timeout_blocker(struct block_info *info) {
    assert(info->type == POLL_TIMEOUT);

    struct timespec now = time_read_clock(CLOCK_MONOTONIC);
    if (time_compare(now, info->poll_timeout_info.end_time) >= 0) {
        return true;
    }

    return poll_blocker_helper(info->poll_timeout_info.nfds, info->poll_timeout_info.fds);
}

int proc_block_poll_timeout(struct task *current, nfds_t nfds, struct pollfd *fds, struct timespec end_time) {
    current->block_info.poll_timeout_info.nfds = nfds;
    current->block_info.poll_timeout_info.fds = fds;
    current->block_info.poll_timeout_info.end_time = end_time;
    current->block_info.type = POLL_TIMEOUT;
    current->block_info.should_unblock = &poll_timeout_blocker;
    return wait_with_blocker(current);
}

static bool waitpid_blocker(struct block_info *info) {
    assert(info->type == WAITPID);
    pid_t pid = info->waitpid_info.pid;
    struct process *waitable_process = NULL;
    int error = proc_get_waitable_process(get_current_process(), pid, &waitable_process);
    if (error) {
        return true;
    }

    return !!waitable_process;
}

int proc_block_waitpid(struct task *current, pid_t pid) {
    current->block_info.waitpid_info.pid = pid;
    current->block_info.type = WAITPID;
    current->block_info.should_unblock = &waitpid_blocker;
    return wait_with_blocker(current);
}
