#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <kernel/fs/file.h>
#include <kernel/hal/output.h>
#include <kernel/proc/process.h>
#include <kernel/net/socket.h>
#include <kernel/net/unix_socket.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static unsigned long socket_id_next = 1;
static spinlock_t id_lock = SPINLOCK_INITIALIZER;

static int socket_file_close(struct file *file);

static struct file_operations socket_file_ops = { 
    socket_file_close, NULL, NULL, NULL
};

static struct hash_map *map;

static int socket_hash(void *i, int num_buckets) {
    return *((unsigned long*) i) % num_buckets;
}

static int socket_equals(void *i1, void *i2) {
    return *((unsigned long*) i1) == *((unsigned long*) i2);
}

static void *socket_key(void *socket) {
    return &((struct socket*) socket)->id;
}

static int socket_file_close(struct file *file) {
    assert(file);

    struct socket_file_data *file_data = file->private_data;
    assert(file_data);

    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    debug_log("Destroying socket: [ %lu ]\n", file_data->socket_id);
    free(socket);
    free(file_data);

    return 0;
}

struct socket *net_create_socket(int domain, int type, int protocol, int *fd) {
    struct process *current = get_current_process();

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (current->files[i] != NULL) {
            current->files[i] = calloc(1, sizeof(struct file));
            current->files[i]->flags = FS_SOCKET;
            current->files[i]->f_op = &socket_file_ops;

            struct socket_file_data *file_data = malloc(sizeof(struct socket_file_data));
            current->files[i]->private_data = file_data;

            spin_lock(&id_lock);
            file_data->socket_id = socket_id_next++;
            spin_unlock(&id_lock);

            struct socket *socket = malloc(sizeof(struct socket));
            socket->domain = domain;
            socket->type = type;
            socket->protocol = protocol;
            socket->id = file_data->socket_id;

            hash_put(map, socket);

            *fd = i;
            return socket;
        }
    }

    *fd = -EMFILE;
    return NULL;
}


int net_socket(int domain, int type, int protocol) {
    switch (domain) {
        case AF_UNIX:
            return net_unix_socket(domain, type, protocol);
        default:
            return -EAFNOSUPPORT;
    }
}

void init_net_sockets() {
    map = hash_create_hash_map(socket_hash, socket_equals, socket_key);
}