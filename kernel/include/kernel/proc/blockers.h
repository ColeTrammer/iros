#ifndef _KERNEL_PROC_BLOCKERS_H
#define _KERNEL_PROC_BLOCKERS_H 1

#include <bits/time_t.h>
#include <stdbool.h>

struct inode;
struct socket;
struct task;

enum block_type {
    SLEEP_MILLISECONDS,
    UNTIL_INODE_IS_READABLE,
    UNTIL_PIPE_IS_READABLE,
    UNTIL_SOCKET_IS_CONNECTED,
    UNTIL_INODE_IS_READABLE_OR_TIMEOUT
};

struct block_info {
    enum block_type type;
    bool (*should_unblock)(struct block_info *info);
    union {
        struct {
            time_t end_time;
        } sleep_milliseconds_info;
        struct {
            struct inode *inode;
        } until_inode_is_readable_info;
        struct {
            struct inode *inode;
        } until_pipe_is_readable_info;
        struct {
            struct socket *socket;
        } until_socket_is_connected_info;
        struct {
            time_t end_time;
            struct inode *inode;
        } until_inode_is_readable_or_timeout_info;
    } __info;
#define sleep_milliseconds_info                 __info.sleep_milliseconds_info
#define until_inode_is_readable_info            __info.until_inode_is_readable_info
#define until_pipe_is_readable_info             __info.until_pipe_is_readable_info
#define until_socket_is_connected_info          __info.until_socket_is_connected_info
#define until_inode_is_readable_or_timeout_info __info.until_inode_is_readable_or_timeout_info
};

void proc_block_sleep_milliseconds(struct task *current, time_t end_time);
void proc_block_until_inode_is_readable(struct task *current, struct inode *inode);
void proc_block_until_pipe_is_readable(struct task *current, struct inode *inode);
void proc_block_until_socket_is_connected(struct task *current, struct socket *socket);
void proc_block_until_inode_is_readable_or_timeout(struct task *current, struct inode *inode, time_t end_time);

#endif /* _KERNEL_PROC_BLOCKERS_H */