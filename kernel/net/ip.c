#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/icmp.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>

// #define IP_V4_DEBUG
#define IP_V4_FRAGMENT_DEBUG

struct ip_v4_fragment_hole {
    uint16_t first;
    uint16_t last;
    uint16_t next;
};

_Static_assert(sizeof(struct ip_v4_fragment_hole) <= 8);

struct ip_v4_fragment_id {
    struct ip_v4_address source;
    struct ip_v4_address dest;
    uint16_t identification;
    uint8_t protocol;
};

struct ip_v4_fragment_desc {
    struct hash_entry hash;
    struct timespec created;
    struct vm_region *reassembly_buffer;
    struct ip_v4_fragment_id id;
    uint16_t hole_list;
};

#define GET_HOLE(buffer, offset)      ((offset) != 0 ? ((struct ip_v4_fragment_hole *) ((buffer) + (offset))) : NULL)
#define GET_HOLE_NUMBER(buffer, hole) ((uintptr_t)(hole) - (uintptr_t)(buffer))
#define MAKE_HOLE(buffer, new_first, new_last, new_next)                                                             \
    ({                                                                                                               \
        uint16_t __new_hole_first = (new_first);                                                                     \
        struct ip_v4_fragment_hole *__new_hole = GET_HOLE((buffer), sizeof(struct ip_v4_packet) + __new_hole_first); \
        __new_hole->first = __new_hole_first;                                                                        \
        __new_hole->last = (new_last);                                                                               \
        __new_hole->next = (new_next);                                                                               \
        __new_hole;                                                                                                  \
    })

static struct hash_map *fragment_store;
static struct timer *fragment_gc_timer;
static struct timespec fragment_max_lifetime = { .tv_sec = 60, .tv_nsec = 0 };

static unsigned int fragment_desc_hash(void *id, int num_buckets) {
    unsigned int sum = 0;
    for (size_t i = 0; i < sizeof(struct ip_v4_fragment_id); i++) {
        sum += ((uint8_t *) id)[i];
    }
    return sum % num_buckets;
}

static int fragment_desc_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct ip_v4_fragment_id)) == 0;
}

static void *fragment_desc_key(struct hash_entry *m) {
    return &hash_table_entry(m, struct ip_v4_fragment_desc)->id;
}

static void free_ip_v4_fragment_desc(struct ip_v4_fragment_desc *desc) {
    if (desc->reassembly_buffer) {
        vm_free_kernel_region(desc->reassembly_buffer);
    }
    free(desc);
}

static void remove_ip_v4_fragment_desc(struct ip_v4_fragment_desc *desc) {
    hash_del(fragment_store, &desc->hash);
    free_ip_v4_fragment_desc(desc);
}

static void gc_fragment(struct hash_entry *entry, void *_now) {
    struct ip_v4_fragment_desc *desc = hash_table_entry(entry, struct ip_v4_fragment_desc);
    struct timespec *now = _now;

    struct timespec delta = time_sub(*now, desc->created);
    if (time_compare(delta, fragment_max_lifetime) >= 0) {
        __hash_del(fragment_store, &desc->id);
        free_ip_v4_fragment_desc(desc);
    }
}

static void gc_fragments(struct timer *timer, void *closure __attribute__((unused))) {
    struct timespec now = time_read_clock(CLOCK_MONOTONIC);
    hash_for_each(fragment_store, gc_fragment, &now);
    __time_reset_kernel_callback(timer, &fragment_max_lifetime);
}

static struct hash_entry *create_fragment_desc(void *_id) {
    struct ip_v4_fragment_id *id = _id;
    struct ip_v4_fragment_desc *desc = malloc(sizeof(struct ip_v4_fragment_desc));
    desc->created = time_read_clock(CLOCK_MONOTONIC);
    desc->id = *id;
    desc->reassembly_buffer = vm_allocate_kernel_region(PAGE_SIZE);

    struct ip_v4_fragment_hole *hole = MAKE_HOLE(desc->reassembly_buffer->start, 0, IP_V4_MAX_PACKET_LENGTH, 0);
    desc->hole_list = GET_HOLE_NUMBER(desc->reassembly_buffer->start, hole);
    return &desc->hash;
}

void handle_fragment(const struct ip_v4_packet *packet) {
    struct ip_v4_fragment_id fragment_id = {
        .source = packet->source,
        .dest = packet->destination,
        .identification = htons(packet->identification),
        .protocol = packet->protocol,
    };
    struct ip_v4_fragment_desc *desc =
        hash_table_entry(hash_put_if_not_present(fragment_store, &fragment_id, create_fragment_desc), struct ip_v4_fragment_desc);

    uint16_t fragment_first = IP_V4_FRAGMENT_OFFSET(packet);
    uint16_t fragment_last = fragment_first + htons(packet->length) - sizeof(struct ip_v4_packet) - 1;
    bool first_fragment = fragment_first == 0;
    bool last_fragment = packet->more_fragments == 0;

#ifdef IP_V4_FRAGMENT_DEBUG
    debug_log("Got fragment: [ %p, %u, %u, %u, %u ]\n", desc, fragment_first, fragment_last, first_fragment, last_fragment);
#endif /* IP_V4_FRAGMENT_DEBUG */

    uint16_t fragment_length = fragment_last - fragment_first + 1;
    if (!last_fragment && fragment_length % 8 != 0) {
        debug_log("Fragment length is not a multiple of 8 octets\n");
        remove_ip_v4_fragment_desc(desc);
        return;
    }

    size_t current_packet_length = fragment_last + 1 + sizeof(struct ip_v4_packet);
    if (current_packet_length > IP_V4_MAX_PACKET_LENGTH) {
        debug_log("Fragment is way too big");
        remove_ip_v4_fragment_desc(desc);
        return;
    }

    // The reassambly buffer needs at least enough space to hold data until the end of the current fragment,
    // plus space to store the hold descriptor at the end of the packet.
    if (current_packet_length + (last_fragment ? 0 : sizeof(struct ip_v4_fragment_hole)) >
        desc->reassembly_buffer->end - desc->reassembly_buffer->start) {
        desc->reassembly_buffer = vm_reallocate_kernel_region(desc->reassembly_buffer, ALIGN_UP(current_packet_length, PAGE_SIZE));
    }

    void *reassembly_buffer = (void *) desc->reassembly_buffer->start;
    struct ip_v4_packet *reassembled_packet = reassembly_buffer;

    // The first fragment fills in all of the header fields except for the length, while the length is only known
    // once the last fragment is recieved.
    if (first_fragment) {
        uint16_t length_save = reassembled_packet->length;
        memcpy(reassembled_packet, packet, sizeof(struct ip_v4_packet));
        reassembled_packet->length = length_save;
        reassembled_packet->more_fragments = 0;
    } else if (last_fragment) {
        reassembled_packet->length = ntohs(current_packet_length);
    }

    struct ip_v4_fragment_hole *prev_hole = (void *) &desc->hole_list - offsetof(struct ip_v4_fragment_hole, next);
    struct ip_v4_fragment_hole *hole = GET_HOLE(reassembly_buffer, prev_hole->next);
    for (; hole; prev_hole = hole, hole = GET_HOLE(reassembly_buffer, hole->next)) {
        uint16_t hole_first = hole->first;
        uint16_t hole_last = hole->last;
        if (fragment_first > hole_last || fragment_last < hole_first) {
            continue;
        }

        // Remove the old hole, since this fragment at least partially fills it in.
        prev_hole->next = hole->next;
        hole = prev_hole;

        // Add a new hole if there's a gap between the hole's left and the fragment's start.
        if (fragment_first > hole_first) {
            struct ip_v4_fragment_hole *new_hole = MAKE_HOLE(reassembly_buffer, hole_first, fragment_first - 1, hole->next);
            hole->next = GET_HOLE_NUMBER(reassembly_buffer, new_hole);
            hole = new_hole;
        }

        // Add a new hole if there's a gap between the fragment's end and the hole's end.
        if (fragment_last < hole_last && !last_fragment) {
            struct ip_v4_fragment_hole *new_hole = MAKE_HOLE(reassembly_buffer, fragment_last + 1, hole_last, hole->next);
            hole->next = GET_HOLE_NUMBER(reassembly_buffer, new_hole);
            hole = new_hole;
        }
    }

    // Fill in the reassembly buffer with the fragment data, now that there cannot be any hole data stored there.
    memcpy(reassembly_buffer + sizeof(struct ip_v4_packet) + fragment_first, packet->payload, fragment_length);

    if (desc->hole_list == 0) {
#ifdef IP_V4_FRAGMENT_DEBUG
        debug_log("Successfully reassembled packet: [ %p ]\n", desc);
#endif /* IP_V4_FRAGMENT_DEBUG */
        // net_ip_v4_recieve(reassembled_packet, htons(reassembled_packet->length));
        remove_ip_v4_fragment_desc(desc);
    }
}

uint8_t net_packet_header_to_ip_v4_type(enum packet_header_type type) {
    switch (type) {
        case PH_ICMP:
            return IP_V4_PROTOCOL_ICMP;
        case PH_UDP:
            return IP_V4_PROTOCOL_UDP;
        case PH_TCP:
            return IP_V4_PROTOCOL_TCP;
        default:
            return 0;
    }
}

enum packet_header_type net_inet_protocol_to_packet_header_type(uint8_t protocol) {
    switch (protocol) {
        case IPPROTO_ICMP:
            return PH_ICMP;
        case IPPROTO_UDP:
            return PH_UDP;
        case IPPROTO_TCP:
            return PH_TCP;
        default:
            return PH_RAW_DATA;
    }
}

int net_send_ip_v4(struct socket *socket, struct network_interface *interface, uint8_t protocol, struct ip_v4_address dest, const void *buf,
                   size_t len) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send IP V4 packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct destination_cache_entry *destination = net_lookup_destination(interface, dest);

    struct ip_v4_address d = dest;
    debug_log("Sending raw IPV4 to: [ %u.%u.%u.%u ]\n", d.addr[0], d.addr[1], d.addr[2], d.addr[3]);

    struct packet *packet = net_create_packet(interface, socket, destination, len);
    packet->header_count = 3;

    struct packet_header *raw_data =
        net_init_packet_header(packet, 2, net_inet_protocol_to_packet_header_type(protocol), packet->inline_data, len);
    memcpy(raw_data->raw_header, buf, len);

    int ret = interface->ops->send_ip_v4(interface, destination, packet);

    net_drop_destination_cache_entry(destination);
    return ret;
}

void net_ip_v4_recieve(struct packet *packet) {
    struct packet_header *header = net_packet_inner_header(packet);
    if (header->length < sizeof(struct ip_v4_packet)) {
        debug_log("IP V4 packet too small\n");
        return;
    }

    struct ip_v4_packet *ip_packet = header->raw_header;

#ifdef IP_V4_DEBUG
    net_ip_v4_log(ip_packet);
#endif /* IP_V4_DEBUG */

    if (ip_packet->more_fragments || IP_V4_FRAGMENT_OFFSET(ip_packet) > 0) {
        // handle_fragment(ip_packet);
        return;
    }

    struct packet_header *next_header = net_packet_add_header(packet, sizeof(struct ip_v4_packet));
    switch (ip_packet->protocol) {
        case IP_V4_PROTOCOL_ICMP: {
            next_header->type = PH_ICMP;
            net_icmp_recieve(packet);
            return;
        }
        case IP_V4_PROTOCOL_UDP: {
            next_header->type = PH_UDP;
            net_udp_recieve(packet);
            return;
        }
        case IP_V4_PROTOCOL_TCP: {
            next_header->type = PH_TCP;
            net_tcp_recieve(packet);
            return;
        }
        default: {
            break;
        }
    }

    debug_log("Ignored packet\n");
}

void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source,
                           struct ip_v4_address dest, const void *payload, uint16_t payload_length) {
    packet->version = IP_V4_VERSION;
    packet->ihl = IP_V4_BYTES_TO_WORDS(sizeof(struct ip_v4_packet));
    packet->dscp = 0;
    packet->ecn = 0;
    packet->length = htons(sizeof(struct ip_v4_packet) + payload_length);
    packet->identification = htons(ident);
    packet->reserved_flag = 0;
    packet->dont_fragment = 0;
    packet->more_fragments = 0;
    packet->fragment_offset_low = 0;
    packet->fragment_offset_high = 0;
    packet->ttl = 64;
    packet->protocol = protocol;
    packet->source = source;
    packet->destination = dest;
    packet->checksum = 0;
    packet->checksum = htons(in_compute_checksum(packet, sizeof(struct ip_v4_packet)));
    if (payload) {
        memcpy(packet->payload, payload, payload_length);
    }
}

void net_ip_v4_log(const struct ip_v4_packet *ip_packet) {
    debug_log("IP v4 Packet:\n"
              "               Header Len   [ %15u ]   Version   [ %15u ]\n"
              "               DSCP         [ %15u ]   ECN       [ %15u ]\n"
              "               Length       [ %15u ]   ID        [ %15u ]\n"
              "               Flags        [ RSZ=%u DF=%u MF=%u ]   Frag Off  [ %15u ]\n"
              "               TTL          [ %15u ]   Protocol  [ %15u ]\n"
              "               Source IP    [ %03u.%03u.%03u.%03u ]   Dest IP   [ %03u.%03u.%03u.%03u ]\n"
              "               Data Len     [ %15u ]   Data off  [ %15u ]\n",
              ip_packet->ihl, ip_packet->version, ip_packet->dscp, ip_packet->ecn, ntohs(ip_packet->length),
              ntohs(ip_packet->identification), ip_packet->reserved_flag, ip_packet->dont_fragment, ip_packet->more_fragments,
              IP_V4_FRAGMENT_OFFSET(ip_packet), ip_packet->ttl, ip_packet->protocol, ip_packet->source.addr[0], ip_packet->source.addr[1],
              ip_packet->source.addr[2], ip_packet->source.addr[3], ip_packet->destination.addr[0], ip_packet->destination.addr[1],
              ip_packet->destination.addr[2], ip_packet->destination.addr[3],
              ntohs(ip_packet->length) - ip_packet->ihl * (uint32_t) sizeof(uint32_t), ip_packet->ihl * (uint32_t) sizeof(uint32_t));
}

void init_ip_v4(void) {
    fragment_store = hash_create_hash_map(fragment_desc_hash, fragment_desc_equals, fragment_desc_key);
    fragment_gc_timer = time_register_kernel_callback(&fragment_max_lifetime, gc_fragments, NULL);
}
