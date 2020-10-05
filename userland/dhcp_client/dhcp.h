#pragma once

#include <netinet/in.h>
#include <stdint.h>
#include <kernel/net/link_layer_address.h>

#define DHCP_MINIMUM_OPTIONS_SIZE 312
#define DHCP_MINIMUM_PACKET_SIZE  576

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define DHCP_MESSAGE_TYPE_DISCOVER 1
#define DHCP_MESSAGE_TYPE_OFFER    2
#define DHCP_MESSAGE_TYPE_REQUEST  3
#define DHCP_MESSAGE_TYPE_DECLINE  4
#define DHCP_MESSAGE_TYPE_ACK      5
#define DHCP_MESSAGE_TYPE_NAK      6
#define DHCP_MESSAGE_TYPE_RELEASE  7

struct network_interface;
struct packet;

struct dhcp_packet {
#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY   2
    uint8_t op;
#define DHCP_HTYPE_ETHERNET 1
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
#define DHCP_FLAG_BROADCAST 1
    uint16_t flags;
    struct in_addr ciaddr;
    struct in_addr yiaddr;
    struct in_addr siaddr;
    struct in_addr giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
#define DHCP_OPTION_MAGIC_COOKIE_1 0x63
#define DHCP_OPTION_MAGIC_COOKIE_2 0x82
#define DHCP_OPTION_MAGIC_COOKIE_3 0x53
#define DHCP_OPTION_MAGIC_COOKIE_4 0x63
#define DHCP_OPTION_PAD            0
#define DHCP_OPTION_SUBNET_MASK    1
#define DHCP_OPTION_ROUTER         3
#define DHCP_OPTION_DNS_SERVERS    6
#define DHCP_OPTION_REQUEST_IP     50
#define DHCP_OPTION_IP_LEASE_TIME  51
#define DHCP_OPTION_MESSAGE_TYPE   53
#define DHCP_OPTION_SERVER_ID      54
#define DHCP_OPTION_END            255
    uint8_t options[];
} __attribute__((packed));

static inline uint8_t net_ll_to_dhcp_hw_type(ll_address_type type) {
    switch (type) {
        case LL_TYPE_ETHERNET:
            return DHCP_HTYPE_ETHERNET;
        default:
            assert(false);
            return 0;
    }
}

static inline ll_address_type net_dhcp_hw_to_ll_type(uint8_t type) {
    switch (type) {
        case DHCP_HTYPE_ETHERNET:
            return LL_TYPE_ETHERNET;
        default:
            return LL_TYPE_NONE;
    }
}
