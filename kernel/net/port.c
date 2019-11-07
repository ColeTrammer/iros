#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/util/hash_map.h>

#define EPHEMERAL_PORT_START 5000
#define PORT_MAX 16000

static struct hash_map *map;

static int hash(void *i, int num_buckets) {
    return *((uint16_t*) i) % num_buckets;
}

static int equals(void *i1, void *i2) {
    return *((uint16_t*) i1) == *((uint16_t*) i2);
}

static void *key(void *p) {
    return &((struct port_to_socket_id*) p)->port;
}

struct socket *net_get_socket_from_port(uint16_t port) {
    struct port_to_socket_id *p = hash_get(map, &port);
    if (p == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(p->socket_id);
}

int net_bind_to_ephemeral_port(unsigned long socket_id) {
    struct port_to_socket_id *p = malloc(sizeof(struct port_to_socket_id));
    p->socket_id = socket_id;

    for (uint16_t port = EPHEMERAL_PORT_START; port < PORT_MAX; port++) {
        // FIXME: data races...
        if (!hash_get(map, port)) {
            p->port = port;
            hash_put(map, p);
            return 0;
        }
    }

    return -EADDRINUSE;
}

void init_ports() {
    map = hash_create_hash_map(hash, equals, key);
}