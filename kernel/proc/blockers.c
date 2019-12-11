#include <assert.h>

#include <kernel/fs/inode.h>
#include <kernel/fs/pipe.h>
#include <kernel/hal/timer.h>
#include <kernel/net/socket.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

static bool sleep_milliseconds_blocker(struct block_info *info) {
    assert(info->type == SLEEP_MILLISECONDS);
    return get_time() >= info->sleep_milliseconds_info.end_time;
}

void proc_block_sleep_milliseconds(struct task *current, time_t end_time) {
    disable_interrupts();
    current->block_info.sleep_milliseconds_info.end_time = end_time;
    current->block_info.type = SLEEP_MILLISECONDS;
    current->block_info.should_unblock = &sleep_milliseconds_blocker;
    current->blocking = true;
    current->sched_state = WAITING;
    __kernel_yield();
}

static bool until_inode_is_readable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_INODE_IS_READABLE);
    return info->until_inode_is_readable_info.inode->readable;
}

void proc_block_until_inode_is_readable(struct task *current, struct inode *inode) {
    disable_interrupts();
    current->block_info.until_inode_is_readable_info.inode = inode;
    current->block_info.type = UNTIL_INODE_IS_READABLE;
    current->block_info.should_unblock = &until_inode_is_readable_blocker;
    current->blocking = true;
    current->sched_state = WAITING;
    __kernel_yield();
}

static bool until_pipe_is_readable_blocker(struct block_info *info) {
    assert(info->type == UNTIL_PIPE_IS_READABLE);

    // Make sure to unblock the process if the other end of the pipe disconnects
    struct inode *inode = info->until_pipe_is_readable_info.inode;
    return inode->readable || !is_pipe_write_end_open(inode);
}

void proc_block_until_pipe_is_readable(struct task *current, struct inode *inode) {
    assert(inode->flags & FS_FIFO);
    disable_interrupts();
    current->block_info.until_pipe_is_readable_info.inode = inode;
    current->block_info.type = UNTIL_PIPE_IS_READABLE;
    current->block_info.should_unblock = &until_pipe_is_readable_blocker;
    current->blocking = true;
    current->sched_state = WAITING;
    __kernel_yield();
}

static bool until_socket_is_connected_blocker(struct block_info *info) {
    assert(info->type == UNTIL_SOCKET_IS_CONNECTED);

    return info->until_socket_is_connected_info.socket->state == CONNECTED;
}

void proc_block_until_socket_is_connected(struct task *current, struct socket *socket) {
    assert(socket->state != CONNECTED);
    disable_interrupts();
    current->block_info.until_socket_is_connected_info.socket = socket;
    current->block_info.type = UNTIL_SOCKET_IS_CONNECTED;
    current->block_info.should_unblock = &until_socket_is_connected_blocker;
    current->blocking = true;
    current->sched_state = WAITING;
    __kernel_yield();
}