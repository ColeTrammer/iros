#ifndef _KERNEL_NET_UMESSAGE_H
#define _KERNEL_NET_UMESSAGE_H 1

#include <sys/umessage.h>

#include <kernel/util/list.h>
#include <kernel/util/ring_buffer.h>
#include <kernel/util/spinlock.h>

#define UMESSAGE_QUEUE_MAX 32

struct socket;

struct queued_umessage {
    int ref_count;
    int flags;
    struct umessage message;
};

struct umessage_queue {
    struct socket *socket;
    struct list_node list;

    // Ring buffer of queued_message pointers
    struct ring_buffer recv_queue;
    spinlock_t recv_queue_lock;
};

struct queued_umessage *net_create_umessage(uint16_t category, uint16_t type, int flags, uint32_t length, const void *data);
struct queued_umessage *net_bump_umessage(struct queued_umessage *umessage);
void net_drop_umessage(struct queued_umessage *umessage);
void net_post_umessage(struct queued_umessage *umessage);

#endif /* _KERNEL_NET_UMESSAGE_H */
