#ifndef _KERNEL_PROC_BLOCKERS_H
#define _KERNEL_PROC_BLOCKERS_H 1

#include <bits/time_t.h>
#include <stdbool.h>
#include <stdint.h>

struct inode;
struct socket;
struct task;

enum block_type {
    SLEEP_MILLISECONDS,
    UNTIL_INODE_IS_READABLE,
    UNTIL_PIPE_IS_READABLE,
    UNTIL_SOCKET_IS_CONNECTED,
    UNTIL_INODE_IS_READABLE_OR_TIMEOUT,
    UNTIL_INODE_IS_WRITABLE,
    UNTIL_SOCKET_HAS_CONNECTION,
    UNTIL_SOCKET_IS_READABLE,
    UNTIL_SOCKET_IS_READABLE_WITH_TIMEOUT,
    SELECT,
    SELECT_TIMEOUT,
    CUSTOM
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
        struct {
            struct inode *inode;
        } until_inode_is_writable_info;
        struct {
            struct socket *socket;
        } until_socket_has_connection_info;
        struct {
            struct socket *socket;
        } until_socket_is_readable_info;
        struct {
            struct socket *socket;
            time_t end_time;
        } until_socket_is_readable_with_timeout_info;
        struct {
            int nfds;
            uint8_t *readfds;
            uint8_t *writefds;
            uint8_t *exceptfds;
        } select_info;
        struct {
            int nfds;
            uint8_t *readfds;
            uint8_t *writefds;
            uint8_t *exceptfds;
            time_t end_time;
        } select_timeout_info;
    } __info;
#define sleep_milliseconds_info                    __info.sleep_milliseconds_info
#define until_inode_is_readable_info               __info.until_inode_is_readable_info
#define until_pipe_is_readable_info                __info.until_pipe_is_readable_info
#define until_socket_is_connected_info             __info.until_socket_is_connected_info
#define until_inode_is_readable_or_timeout_info    __info.until_inode_is_readable_or_timeout_info
#define until_inode_is_writable_info               __info.until_inode_is_writable_info
#define until_socket_has_connection_info           __info.until_socket_has_connection_info
#define until_socket_is_readable_info              __info.until_socket_is_readable_info
#define until_socket_is_readable_with_timeout_info __info.until_socket_is_readable_with_timeout_info
#define select_info                                __info.select_info
#define select_timeout_info                        __info.select_timeout_info
};

void proc_block_sleep_milliseconds(struct task *current, time_t end_time);
void proc_block_until_inode_is_readable(struct task *current, struct inode *inode);
void proc_block_until_pipe_is_readable(struct task *current, struct inode *inode);
void proc_block_until_socket_is_connected(struct task *current, struct socket *socket);
void proc_block_until_inode_is_readable_or_timeout(struct task *current, struct inode *inode, time_t end_time);
void proc_block_until_inode_is_writable(struct task *current, struct inode *inode);
void proc_block_custom(struct task *current);
void proc_block_until_socket_has_connection(struct task *current, struct socket *socket);
void proc_block_until_socket_is_readable(struct task *current, struct socket *socket);
void proc_block_until_socket_is_readable_with_timeout(struct task *current, struct socket *socket, time_t end_time);
void proc_block_select(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds);
void proc_block_select_timeout(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds, time_t end_time);

#endif /* _KERNEL_PROC_BLOCKERS_H */