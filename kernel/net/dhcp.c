#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/dhcp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>

void net_configure_interface_with_dhcp(struct network_interface *interface) {
    size_t dhcp_length = DHCP_MINIMUM_PACKET_SIZE;

    struct mac_address our_mac_address = interface->ops->get_mac_address(interface);
    struct ethernet_packet *packet = net_create_ethernet_packet(our_mac_address, MAC_BROADCAST, ETHERNET_TYPE_IPV4, dhcp_length);

    struct ip_v4_packet *ip_packet = (struct ip_v4_packet *) packet->payload;
    net_init_ip_v4_packet(ip_packet, 1, IP_V4_PROTOCOL_UDP, IP_V4_ZEROES, IP_V4_BROADCAST, dhcp_length - sizeof(struct ip_v4_packet));

    size_t udp_packet_length = dhcp_length - sizeof(struct ip_v4_packet);
    struct udp_packet *udp_packet = (struct udp_packet *) ip_packet->payload;
    net_init_udp_packet(udp_packet, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, udp_packet_length - sizeof(struct udp_packet), NULL);

    struct dhcp_packet *data = (struct dhcp_packet *) udp_packet->payload;
    data->op = DHCP_OP_REQUEST;
    data->htype = DHCP_HTYPE_ETHERNET;
    data->hlen = sizeof(struct mac_address);
    data->hops = 0;
    data->xid = htonl(0x12345678); // FIXME: This should be randomly generated
    data->secs = htons(0);
    data->flags = htons(0);
    data->ciaddr = IP_V4_ZEROES;
    data->yiaddr = IP_V4_ZEROES;
    data->siaddr = IP_V4_ZEROES;
    data->giaddr = IP_V4_ZEROES;
    memcpy(data->chaddr, &our_mac_address, sizeof(our_mac_address));
    memset(data->sname, 0, sizeof(data->sname));
    memset(data->file, 0, sizeof(data->file));
    data->options[0] = DHCP_OPTION_MAGIC_COOKIE_1;
    data->options[1] = DHCP_OPTION_MAGIC_COOKIE_2;
    data->options[2] = DHCP_OPTION_MAGIC_COOKIE_3;
    data->options[3] = DHCP_OPTION_MAGIC_COOKIE_4;
    data->options[4] = DHCP_OPTION_MESSAGE_TYPE;
    data->options[5] = 1;
    data->options[6] = DHCP_MESSAGE_TYPE_DISCOVER;

    struct ip_v4_pseudo_header header = { IP_V4_ZEROES, IP_V4_BROADCAST, 0, IP_V4_PROTOCOL_UDP, htons(udp_packet_length) };
    udp_packet->checksum = ntohs(
        in_compute_checksum_with_start(udp_packet, udp_packet_length, in_compute_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    debug_log("Sending DHCP DISCOVER packet\n");

    assert(interface->ops->send(interface, packet, dhcp_length + sizeof(struct ethernet_packet)) > 0);
    free(packet);
}
