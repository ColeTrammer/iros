#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/umessage.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>
#include <kernel/util/mutex.h>

static mutex_t queue_list_lock = MUTEX_INITIALIZER;
static struct list_node queue_list = INIT_LIST(queue_list);

static struct umessage_queue *create_umessage_queue(struct socket *socket) {
    struct umessage_queue *ret = socket->private_data = malloc(sizeof(struct umessage_queue));
    ret->socket = socket;
    init_list(&ret->list);
    init_ring_buffer(&ret->recv_queue, UMESSAGE_QUEUE_MAX * sizeof(struct umessage_queue *));
    init_spinlock(&ret->recv_queue_lock);
    return ret;
}

static void free_umessage_queue(struct umessage_queue *queue) {
    struct queued_umessage *message;
    while (ring_buffer_size(&queue->recv_queue) >= sizeof(*message)) {
        ring_buffer_read(&queue->recv_queue, &message, sizeof(message));
        net_drop_umessage(message);
    }
    kill_ring_buffer(&queue->recv_queue);
    free(queue);
}

static void register_umessage_queue(struct umessage_queue *queue) {
    mutex_lock(&queue_list_lock);
    list_append(&queue_list, &queue->list);
    mutex_unlock(&queue_list_lock);
}

static void unregister_umessage_queue(struct umessage_queue *queue) {
    mutex_lock(&queue_list_lock);
    list_remove(&queue->list);
    mutex_unlock(&queue_list_lock);
}

struct queued_umessage *net_create_umessage(uint16_t category, uint16_t type, int flags, uint32_t length, const void *data) {
    struct queued_umessage *message = malloc(sizeof(struct queued_umessage) - sizeof(struct umessage) + length);
    message->ref_count = 1;
    message->flags = flags;
    message->message.length = length;
    message->message.category = category;
    message->message.type = type;
    if (data) {
        memcpy(message->message.data, data, length - sizeof(struct umessage));
    }
    return message;
}

struct queued_umessage *net_bump_umessage(struct queued_umessage *umessage) {
    atomic_fetch_add(&umessage->ref_count, 1);
    return umessage;
}

void net_drop_umessage(struct queued_umessage *umessage) {
    if (atomic_fetch_sub(&umessage->ref_count, 1) == 1) {
        free(umessage);
    }
}

void net_post_umessage(struct queued_umessage *umessage) {
    mutex_lock(&queue_list_lock);
    list_for_each_entry(&queue_list, queue, struct umessage_queue, list) {
        spin_lock(&queue->recv_queue_lock);
        if (!ring_buffer_full(&queue->recv_queue)) {
            queue->socket->readable = true;
            ring_buffer_write(&queue->recv_queue, net_bump_umessage(umessage), sizeof(umessage));
        }
        spin_unlock(&queue->recv_queue_lock);
    }
    mutex_unlock(&queue_list_lock);
}

static int umessage_close(struct socket *socket) {
    struct umessage_queue *queue = socket->private_data;
    if (queue) {
        unregister_umessage_queue(queue);
        free_umessage_queue(queue);
    }
    return 0;
}

static ssize_t umessage_recvfrom(struct socket *socket, void *buffer, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    (void) flags;
    (void) addr;
    (void) addrlen;

    struct timespec start_time = time_read_clock(CLOCK_MONOTONIC);
    struct umessage_queue *queue = socket->private_data;

    struct queued_umessage *umessage = NULL;
    for (;;) {
        spin_lock(&queue->recv_queue_lock);
        if (!ring_buffer_empty(&queue->recv_queue)) {
            ring_buffer_read(&queue->recv_queue, &umessage, sizeof(umessage));
        }
        spin_unlock(&queue->recv_queue_lock);

        if (umessage) {
            ssize_t ret = MIN(umessage->message.length, len);
            memcpy(buffer, &umessage->message, ret);
            net_drop_umessage(umessage);
            return ret;
        }

        if (socket->type & SOCK_NONBLOCK) {
            return -EAGAIN;
        }

        int ret = net_block_until_socket_is_readable(socket, start_time);
        if (ret) {
            return ret;
        }
    }
}

static struct socket_ops umessage_ops = {
    .close = umessage_close,
    .getsockopt = net_generic_getsockopt,
    .setsockopt = net_generic_setsockopt,
    .recvfrom = umessage_recvfrom,
    .sendto = NULL,
};

static int umessage_socket(int domain, int type, int protocol) {
    int fd;
    struct socket *socket = net_create_socket_fd(domain, type, protocol, &umessage_ops, &fd, NULL);
    struct umessage_queue *queue = create_umessage_queue(socket);
    register_umessage_queue(queue);
    return fd;
}

static struct socket_protocol umessage_protocol = {
    .domain = AF_UMESSAGE,
    .type = SOCK_DGRAM,
    .protocol = 0,
    .is_default_protocol = true,
    .name = "UMessage",
    .create_socket = umessage_socket,
};

static void init_umessage(void) {
    net_register_protocol(&umessage_protocol);
}
INIT_FUNCTION(init_umessage, net);
