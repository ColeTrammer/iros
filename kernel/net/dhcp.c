#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/dhcp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/util/random.h>

static uint8_t net_ll_to_dhcp_hw_type(enum ll_address_type type) {
    switch (type) {
        case LL_TYPE_ETHERNET:
            return DHCP_HTYPE_ETHERNET;
        default:
            assert(false);
            return 0;
    }
}

static enum ll_address_type net_dhcp_hw_to_ll_type(uint8_t type) {
    switch (type) {
        case DHCP_HTYPE_ETHERNET:
            return LL_TYPE_ETHERNET;
        default:
            return LL_TYPE_NONE;
    }
}

static void net_send_dhcp(struct network_interface *interface, uint8_t message_type, struct link_layer_address our_address,
                          struct ip_v4_address client_ip, struct ip_v4_address server_ip) {
    size_t total_length = DHCP_MINIMUM_PACKET_SIZE;
    size_t udp_length = total_length - sizeof(struct ip_v4_packet);
    size_t dhcp_length = udp_length - sizeof(struct udp_packet);

    struct packet *packet = net_create_packet(interface, NULL, NULL, udp_length);
    packet->header_count = 4;

    struct packet_header *udp_header = net_init_packet_header(packet, 2, PH_UDP, packet->inline_data, sizeof(struct udp_packet));
    struct udp_packet *udp_packet = udp_header->raw_header;
    net_init_udp_packet(udp_packet, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, dhcp_length, NULL);

    struct packet_header *dhcp_header = net_init_packet_header(packet, 3, PH_RAW_DATA, udp_packet->payload, dhcp_length);
    struct dhcp_packet *data = dhcp_header->raw_header;
    data->op = DHCP_OP_REQUEST;
    data->htype = net_ll_to_dhcp_hw_type(our_address.type);
    data->hlen = our_address.length;
    data->hops = 0;
    data->xid = htonl(interface->config_context.xid = get_random_bytes());
    data->secs = htons(0);
    data->flags = htons(0);
    data->ciaddr = client_ip;
    data->yiaddr = IP_V4_ZEROES;
    data->siaddr = server_ip;
    data->giaddr = IP_V4_ZEROES;
    memcpy(data->chaddr, &our_address.addr, our_address.length);
    memset(data->sname, 0, sizeof(data->sname));
    memset(data->file, 0, sizeof(data->file));

    size_t opt_start = 0;
    data->options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_1;
    data->options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_2;
    data->options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_3;
    data->options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_4;
    data->options[opt_start++] = DHCP_OPTION_MESSAGE_TYPE;
    data->options[opt_start++] = 1;
    data->options[opt_start++] = message_type;

    if (client_ip.addr[0] != 0 || client_ip.addr[1] != 0 || client_ip.addr[2] != 0 || client_ip.addr[3] != 0) {
        data->options[opt_start++] = DHCP_OPTION_REQUEST_IP;
        data->options[opt_start++] = sizeof(struct ip_v4_address);
        data->options[opt_start++] = client_ip.addr[0];
        data->options[opt_start++] = client_ip.addr[1];
        data->options[opt_start++] = client_ip.addr[2];
        data->options[opt_start++] = client_ip.addr[3];
    }

    if (server_ip.addr[0] != 0 || server_ip.addr[1] != 0 || server_ip.addr[2] != 0 || server_ip.addr[3] != 0) {
        data->options[opt_start++] = DHCP_OPTION_SERVER_ID;
        data->options[opt_start++] = sizeof(struct ip_v4_address);
        data->options[opt_start++] = server_ip.addr[0];
        data->options[opt_start++] = server_ip.addr[1];
        data->options[opt_start++] = server_ip.addr[2];
        data->options[opt_start++] = server_ip.addr[3];
    }
    data->options[opt_start] = DHCP_OPTION_END;

    struct ip_v4_pseudo_header header = { IP_V4_ZEROES, IP_V4_BROADCAST, 0, IP_V4_PROTOCOL_UDP, htons(udp_length) };
    udp_packet->checksum =
        ntohs(in_compute_checksum_with_start(udp_packet, udp_length, in_compute_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    debug_log("Sending DHCP DISCOVER packet: [ %u ]\n", interface->config_context.xid);

    assert(interface->ops->send_ip_v4(interface, NULL, packet) == 0);
}

void net_configure_interface_with_dhcp(struct network_interface *interface) {
    struct link_layer_address our_address = interface->link_layer_address;
    net_send_dhcp(interface, DHCP_MESSAGE_TYPE_DISCOVER, our_address, IP_V4_ZEROES, IP_V4_ZEROES);
}

void net_dhcp_recieve(struct packet *net_packet) {
    struct packet_header *dhcp_header = net_packet_inner_header(net_packet);
    if (dhcp_header->length < DHCP_MINIMUM_PACKET_SIZE - sizeof(struct udp_packet) - sizeof(struct ip_v4_packet)) {
        debug_log("dhcp packet too small: [ %u ]\n", dhcp_header->length);
        return;
    }

    struct dhcp_packet *packet = dhcp_header->raw_header;
    debug_log("recieved DHCP packet: [ %u ]\n", ntohl(packet->xid));

    if (packet->op != DHCP_OP_REPLY) {
        debug_log("ignoring dhcp request packet\n");
        return;
    }

    struct network_interface *interface = NULL;
    net_for_each_interface(iter) {
        if (iter->config_context.state != INITIALIZED && iter->config_context.xid == ntohl(packet->xid)) {
            interface = iter;
            break;
        }
    }
    if (!interface) {
        debug_log("DHCP packet has no matching interface\n");
        return;
    }

    debug_log("DHCP packet is for interface: [ %s ]\n", interface->name);

    debug_log("DHCP packet ciaddr: [ %u.%u.%u.%u ]\n", packet->ciaddr.addr[0], packet->ciaddr.addr[1], packet->ciaddr.addr[2],
              packet->ciaddr.addr[3]);
    debug_log("DHCP packet yiaddr: [ %u.%u.%u.%u ]\n", packet->yiaddr.addr[0], packet->yiaddr.addr[1], packet->yiaddr.addr[2],
              packet->yiaddr.addr[3]);
    debug_log("DHCP packet siaddr: [ %u.%u.%u.%u ]\n", packet->siaddr.addr[0], packet->siaddr.addr[1], packet->siaddr.addr[2],
              packet->siaddr.addr[3]);
    debug_log("DHCP packet giaddr: [ %u.%u.%u.%u ]\n", packet->giaddr.addr[0], packet->giaddr.addr[1], packet->giaddr.addr[2],
              packet->giaddr.addr[3]);

    if (packet->options[0] != DHCP_OPTION_MAGIC_COOKIE_1 || packet->options[1] != DHCP_OPTION_MAGIC_COOKIE_2 ||
        packet->options[2] != DHCP_OPTION_MAGIC_COOKIE_3 || packet->options[3] != DHCP_OPTION_MAGIC_COOKIE_4) {
        debug_log("DHCP packet has wrong options magic cookie\n");
        return;
    }

    int dhcp_message_type = -1;
    struct ip_v4_address server_ip;

    size_t i = 4;
    while (i < sizeof(struct dhcp_packet) + dhcp_header->length && packet->options[i] != DHCP_OPTION_END) {
        if (packet->options[i] == DHCP_OPTION_PAD) {
            i++;
            continue;
        }

        uint8_t option_type = packet->options[i];
        uint8_t option_length = packet->options[i + 1];
        const uint8_t *option_data = &packet->options[i + 2];
        switch (option_type) {
            case DHCP_OPTION_SUBNET_MASK:
                if (option_length != 4) {
                    debug_log("DHCP subnet mask has invalid length: [ %hhu ]\n", option_length);
                    return;
                }
                interface->mask.addr[0] = option_data[0];
                interface->mask.addr[1] = option_data[1];
                interface->mask.addr[2] = option_data[2];
                interface->mask.addr[3] = option_data[3];
                debug_log("DHCP subnet mask detected: [ %u.%u.%u.%u ]\n", option_data[0], option_data[1], option_data[2], option_data[3]);
                break;
            case DHCP_OPTION_ROUTER:
                if (option_length % 4 != 0 || option_length == 0) {
                    debug_log("DHCP router list has invalid length: [ %hhu ]\n", option_length);
                    return;
                }
                interface->default_gateway.addr[0] = option_data[0];
                interface->default_gateway.addr[1] = option_data[1];
                interface->default_gateway.addr[2] = option_data[2];
                interface->default_gateway.addr[3] = option_data[3];
                debug_log("DHCP router detected: [ %u.%u.%u.%u ]\n", option_data[0], option_data[1], option_data[2], option_data[3]);
                break;
            case DHCP_OPTION_DNS_SERVERS:
                if (option_length % 4 != 0 || option_length == 0) {
                    debug_log("DHCP dns server list has invalid length: [ %hhu ]\n", option_length);
                    return;
                }
                debug_log("DHCP dns server detected (but will be ignored): [ %u.%u.%u.%u ]\n", option_data[0], option_data[1],
                          option_data[2], option_data[3]);
                break;
            case DHCP_OPTION_IP_LEASE_TIME: {
                if (option_length != 4) {
                    debug_log("DHCP IP address lease time has invalid length: [ %hhu ]\n", option_length);
                    return;
                }
                uint32_t lease_time = ntohl(*((uint32_t *) option_data));
                debug_log("DHCP IP address has lease time (seconds): [ %u ]\n", lease_time);
                break;
            }
            case DHCP_OPTION_MESSAGE_TYPE:
                if (option_length != 1) {
                    debug_log("DHCP message type option has invalid length: [ %hhu ]\n", option_length);
                    return;
                }
                dhcp_message_type = option_data[0];
                break;
            case DHCP_OPTION_SERVER_ID:
                if (option_length != 4) {
                    debug_log("DHCP server id has invalid length: [ %huu ]\n", option_length);
                    return;
                }
                server_ip.addr[0] = option_data[0];
                server_ip.addr[1] = option_data[1];
                server_ip.addr[2] = option_data[2];
                server_ip.addr[3] = option_data[3];
                debug_log("DHCP server ip address detected: [ %u.%u.%u.%u ]\n", option_data[0], option_data[1], option_data[2],
                          option_data[3]);
                break;
            default:
                debug_log("DHCP packet has unknown option (ignoring): [ %hhu, %hhu ]\n", option_type, option_length);
                break;
        }

        i += option_length + 2;
    }

    if (dhcp_message_type == -1) {
        debug_log("DHCP packet has no type\n");
        return;
    }

    struct link_layer_address our_address = interface->link_layer_address;
    struct link_layer_address chaddr = { .type = net_dhcp_hw_to_ll_type(packet->htype), .length = packet->hlen };
    memcpy(chaddr.addr, packet->chaddr, sizeof(packet->chaddr));
    if (!net_link_layer_address_equals(our_address, chaddr)) {
        debug_log("DHCP packet has wrong hard ware address\n");
        return;
    }

    switch (dhcp_message_type) {
        case DHCP_MESSAGE_TYPE_OFFER:
            net_send_dhcp(interface, DHCP_MESSAGE_TYPE_REQUEST, our_address, packet->yiaddr, server_ip);
            break;
        case DHCP_MESSAGE_TYPE_ACK:
            interface->address = packet->yiaddr;
            interface->config_context.state = INITIALIZED;
            debug_log("DHCP bound interface to ip: [ %s, %u.%u.%u.%u ]\n", interface->name, interface->address.addr[0],
                      interface->address.addr[1], interface->address.addr[2], interface->address.addr[3]);
            break;
        default:
            debug_log("ignoring DHCP message type: [ %d ]\n", dhcp_message_type);
            break;
    }
}
