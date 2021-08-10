#ifndef _KERNEL_NET_UMESSAGE_H
#define _KERNEL_NET_UMESSAGE_H 1

#include <sys/umessage.h>

#include <kernel/util/list.h>
#include <kernel/util/ring_buffer.h>
#include <kernel/util/spinlock.h>

#define UMESSAGE_QUEUE_MAX 32

struct socket;
struct socket_protocol;
struct umessage_queue;

struct queued_umessage {
    int ref_count;
    int flags;
    struct umessage message;
};

struct umessage_category {
    uint16_t category;
    uint16_t request_type_count;
    const char *name;
    size_t private_data_size;
    int (*recv)(struct umessage_queue *queue, const struct umessage *umessage);
    void (*init)(struct umessage_queue *queue);
    void (*kill)(struct umessage_queue *queue);
    struct socket_protocol *protocol;
};

struct umessage_queue {
    struct socket *socket;
    struct list_node list;
    struct umessage_category *desc;

    // Ring buffer of queued_message pointers
    struct ring_buffer recv_queue;
    spinlock_t recv_queue_lock;
    uint16_t category;
    char private_data[0] __attribute__((aligned(sizeof(void *))));
};

static inline void *net_umessage_queue_private(struct umessage_queue *queue) {
    return (void *) queue->private_data;
}

struct umessage_queue *net_bump_umessage_queue(struct umessage_queue *queue);
void net_drop_umessage_queue(struct umessage_queue *queue);

struct queued_umessage *net_create_umessage(uint16_t category, uint16_t type, int flags, uint32_t length, const void *data);
struct queued_umessage *net_bump_umessage(struct queued_umessage *umessage);
void net_drop_umessage(struct queued_umessage *umessage);
void net_post_umessage(struct queued_umessage *umessage);
void net_post_umessage_to(struct umessage_queue *queue, struct queued_umessage *umessage);

void net_register_umessage_category(struct umessage_category *category);

#endif /* _KERNEL_NET_UMESSAGE_H */
