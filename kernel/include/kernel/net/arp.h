#ifndef _KERNEL_NET_ARP_H
#define _KERNEL_NET_ARP_H 1

#include <kernel/net/ip_address.h>
#include <kernel/net/link_layer_address.h>

#define ARP_PROTOCOL_TYPE_ETHERNET 1
#define ARP_PROTOCOL_TYPE_IP_V4    0x0800

#define ARP_OPERATION_REQUEST 1
#define ARP_OPERATION_REPLY   2

struct network_interface;

struct arp_packet {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_addr_len;
    uint8_t protocol_addr_len;
    uint16_t operation;
    uint8_t addrs[0];
} __attribute__((packed));

#define ARP_SENDER_HW_ADDR(p)    ((p)->addrs)
#define ARP_SENDER_PROTO_ADDR(p) ((p)->addrs + (p)->hardware_addr_len)
#define ARP_TARGET_HW_ADDR(p)    ((p)->addrs + (p)->hardware_addr_len + (p)->protocol_addr_len)
#define ARP_TARGET_PROTO_ADDR(p) ((p)->addrs + (p)->hardware_addr_len + (p)->protocol_addr_len + (p)->hardware_addr_len)

void net_send_arp_request(struct network_interface *interface, struct ip_v4_address ip_address);

void net_arp_recieve(const struct arp_packet *packet, size_t len);

struct network_data *net_create_arp_packet(struct network_interface *interface, uint16_t op, struct link_layer_address s_addr,
                                           struct ip_v4_address s_ip, struct link_layer_address t_addr, struct ip_v4_address t_ip);
void net_init_arp_packet(struct arp_packet *buf, uint16_t op, struct link_layer_address s_addr, struct ip_v4_address s_ip,
                         struct link_layer_address t_addr, struct ip_v4_address t_ip);

#endif /* _KERNEL_NET_ARP_H */
