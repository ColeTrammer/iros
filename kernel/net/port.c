#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/mutex.h>

#define EPHEMERAL_PORT_START 49152U
#define PORT_MAX             65535U

static struct hash_map *map;

HASH_DEFINE_FUNCTIONS(port, struct port_to_socket_id, uint16_t, port)

struct socket *net_get_socket_from_port(uint16_t port) {
    struct port_to_socket_id *p = hash_get_entry(map, &port, struct port_to_socket_id);
    if (p == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(p->socket_id);
}

static mutex_t port_search_lock = MUTEX_INITIALIZER;

int net_bind_to_ephemeral_port(unsigned long socket_id, uint16_t *port_p) {
    struct port_to_socket_id *p = malloc(sizeof(struct port_to_socket_id));
    p->socket_id = socket_id;

    mutex_lock(&port_search_lock);

    for (uint16_t port = EPHEMERAL_PORT_START; port < PORT_MAX; port++) {
        if (!hash_get(map, &port)) {
            p->port = port;
            hash_put(map, &p->hash);
            mutex_unlock(&port_search_lock);

            debug_log("Bound socket to ephemeral port: [ %lu, %u ]\n", socket_id, port);

            *port_p = port;
            return 0;
        }
    }

    mutex_unlock(&port_search_lock);
    return -EADDRINUSE;
}

int net_bind_to_port(unsigned long socket_id, uint16_t port) {
    struct port_to_socket_id *p = malloc(sizeof(struct port_to_socket_id));
    p->socket_id = socket_id;
    p->port = port;

    mutex_lock(&port_search_lock);

    if (!hash_get(map, &port)) {
        hash_put(map, &p->hash);

        mutex_unlock(&port_search_lock);
        return 0;
    }

    mutex_unlock(&port_search_lock);
    free(p);
    return -EADDRINUSE;
}

void net_unbind_port(uint16_t port) {
    struct port_to_socket_id *p = hash_del_entry(map, &port, struct port_to_socket_id);
    free(p);
}

void init_ports() {
    map = hash_create_hash_map(port_hash, port_equals, port_key);
}
