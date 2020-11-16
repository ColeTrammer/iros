#include <arpa/inet.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "interface.h"

SharedPtr<Interface> Interface::create(const umessage_interface_desc& desc, uint32_t dhcp_xid) {
    auto socket = App::UdpSocket::create(nullptr);
    if (!socket->bind_to_device(desc.name) || !socket->enable_broadcast() || !socket->enable_reuse_addr()) {
        return nullptr;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DHCP_CLIENT_PORT),
        .sin_addr = { .s_addr = INADDR_ANY },
        .sin_zero = { 0 },
    };
    if (!socket->bind(addr)) {
        return nullptr;
    }

    auto interface = make_shared<Interface>(desc, dhcp_xid, socket);

    socket->set_selected_events(App::NotifyWhen::Readable);
    socket->enable_notifications();
    socket->on_ready_to_recieve = [socket, interface] {
        dhcp_packet packet;
        while (const_cast<App::UdpSocket&>(*socket).recvfrom((uint8_t*) &packet, sizeof(packet)) > 0) {
            if (packet.op != DHCP_OP_REPLY) {
                syslog(LOG_INFO, "Ignoring DHCP packet with wrong DHCP op `%u'", packet.op);
                return;
            }

            auto xid = htonl(packet.xid);
            if (interface->m_dhcp_xid != ntohl(packet.xid)) {
                syslog(LOG_INFO, "Ignoring DHCP packet with wrong xid: `%#.8X'", xid);
                return;
            }

            const_cast<Interface&>(*interface).handle_dhcp_response(packet);
        }
    };

    return interface;
}

Interface::Interface(const umessage_interface_desc& desc, uint32_t dhcp_xid, SharedPtr<App::UdpSocket> socket)
    : m_link_layer_address(desc.link_layer_address), m_dhcp_socket(socket), m_name(desc.name), m_dhcp_xid(dhcp_xid) {
    m_state.base.length = sizeof(m_state);
    m_state.base.category = UMESSAGE_INTERFACE;
    m_state.base.type = UMESSAGE_INTERFACE_SET_STATE_REQUEST;
    m_state.interface_index = desc.index;
    m_state.set_default_gateway = false;
    m_state.set_address = false;
    m_state.set_subnet_mask = false;
    m_state.set_flags = false;
    m_state.address.s_addr = INADDR_ANY;
    m_state.flags = desc.flags;
}

bool Interface::send_dhcp(uint8_t message_type, in_addr_t server_ip) {
    dhcp_packet data;
    data.op = DHCP_OP_REQUEST;
    data.htype = net_ll_to_dhcp_hw_type(m_link_layer_address.type);
    data.hlen = m_link_layer_address.length;
    data.hops = 0;
    data.xid = htonl(m_dhcp_xid);
    data.secs = htons(0);
    data.flags = htons(0);
    data.ciaddr = m_state.address;
    data.yiaddr.s_addr = INADDR_ANY;
    data.siaddr.s_addr = server_ip;
    data.giaddr.s_addr = INADDR_ANY;
    memcpy(data.chaddr, &m_link_layer_address.addr, m_link_layer_address.length);
    memset(data.sname, 0, sizeof(data.sname));
    memset(data.file, 0, sizeof(data.file));

    size_t opt_start = 0;
    data.options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_1;
    data.options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_2;
    data.options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_3;
    data.options[opt_start++] = DHCP_OPTION_MAGIC_COOKIE_4;
    data.options[opt_start++] = DHCP_OPTION_MESSAGE_TYPE;
    data.options[opt_start++] = 1;
    data.options[opt_start++] = message_type;

    if (m_state.address.s_addr != INADDR_ANY) {
        data.options[opt_start++] = DHCP_OPTION_REQUEST_IP;
        data.options[opt_start++] = sizeof(in_addr);
        *(in_addr*) &data.options[opt_start] = m_state.address;
        opt_start += sizeof(in_addr);
    }

    if (server_ip != INADDR_ANY) {
        data.options[opt_start++] = DHCP_OPTION_SERVER_ID;
        data.options[opt_start++] = sizeof(in_addr);
        *(in_addr_t*) &data.options[opt_start] = server_ip;
        opt_start += sizeof(in_addr);
    }
    data.options[opt_start] = DHCP_OPTION_END;

    sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port = htons(DHCP_SERVER_PORT),
        .sin_addr = { .s_addr = INADDR_NONE },
        .sin_zero = { 0 },
    };
    return m_dhcp_socket->sendto((uint8_t*) &data, sizeof(data), dest) == sizeof(data);
}

void Interface::handle_dhcp_response(const dhcp_packet& packet) {
    syslog(LOG_INFO, "DHCP packet is for interface `%s'", m_name.string());
    syslog(LOG_INFO, "DHCP packet ciaddr is `%s'", inet_ntoa(packet.ciaddr));
    syslog(LOG_INFO, "DHCP packet yiaddr is `%s'", inet_ntoa(packet.yiaddr));
    syslog(LOG_INFO, "DHCP packet siaddr is `%s'", inet_ntoa(packet.siaddr));
    syslog(LOG_INFO, "DHCP packet giaddr is `%s'", inet_ntoa(packet.giaddr));

    if (packet.options[0] != DHCP_OPTION_MAGIC_COOKIE_1 || packet.options[1] != DHCP_OPTION_MAGIC_COOKIE_2 ||
        packet.options[2] != DHCP_OPTION_MAGIC_COOKIE_3 || packet.options[3] != DHCP_OPTION_MAGIC_COOKIE_4) {
        syslog(LOG_INFO, "DHCP packet has wrong options magic cookie");
        return;
    }

    link_layer_address chaddr = { .type = net_dhcp_hw_to_ll_type(packet.htype), .length = packet.hlen, .addr = {} };
    memcpy(chaddr.addr, packet.chaddr, sizeof(packet.chaddr));
    if (!net_link_layer_address_equals(m_link_layer_address, chaddr)) {
        syslog(LOG_INFO, "DHCP packet has wrong hardware address");
        return;
    }

    int dhcp_message_type = -1;
    in_addr server_ip;

    size_t i = 4;
    while (i < sizeof(dhcp_packet) && packet.options[i] != DHCP_OPTION_END) {
        if (packet.options[i] == DHCP_OPTION_PAD) {
            i++;
            continue;
        }

        uint8_t option_type = packet.options[i];
        uint8_t option_length = packet.options[i + 1];
        const uint8_t* option_data = &packet.options[i + 2];
        switch (option_type) {
            case DHCP_OPTION_SUBNET_MASK:
                if (option_length != 4) {
                    syslog(LOG_INFO, "DHCP subnet mask has invalid length of `%hhu'", option_length);
                    return;
                }
                m_state.subnet_mask = *(const in_addr*) option_data;
                m_state.set_subnet_mask = true;
                syslog(LOG_INFO, "DHCP subnet mask is `%s'", inet_ntoa(m_state.subnet_mask));
                break;
            case DHCP_OPTION_ROUTER:
                if (option_length % 4 != 0 || option_length == 0) {
                    syslog(LOG_INFO, "DHCP router list has invalid length of `%hhu'", option_length);
                    return;
                }
                m_state.default_gateway = *(const in_addr*) option_data;
                m_state.set_default_gateway = true;
                syslog(LOG_INFO, "DHCP router is `%s'", inet_ntoa(m_state.default_gateway));
                break;
            case DHCP_OPTION_DNS_SERVERS:
                if (option_length % 4 != 0 || option_length == 0) {
                    syslog(LOG_INFO, "DHCP dns server list has invalid length of `%hhu'", option_length);
                    return;
                }
                syslog(LOG_INFO, "DHCP dns server detected (but will be ignored) of `%s'", inet_ntoa(*(const in_addr*) option_data));
                break;
            case DHCP_OPTION_IP_LEASE_TIME: {
                if (option_length != 4) {
                    syslog(LOG_INFO, "DHCP IP address lease time has invalid length of `%hhu'", option_length);
                    return;
                }
                uint32_t lease_time = ntohl(*((uint32_t*) option_data));
                syslog(LOG_INFO, "DHCP IP address has lease time of %u seconds", lease_time);
                break;
            }
            case DHCP_OPTION_MESSAGE_TYPE:
                if (option_length != 1) {
                    syslog(LOG_INFO, "DHCP message type option has invalid length of `%hhu'", option_length);
                    return;
                }
                dhcp_message_type = option_data[0];
                break;
            case DHCP_OPTION_SERVER_ID:
                if (option_length != 4) {
                    syslog(LOG_INFO, "DHCP server id has invalid length of `%huu'", option_length);
                    return;
                }
                server_ip = *(const in_addr*) option_data;
                syslog(LOG_INFO, "DHCP server ip address is `%s'", inet_ntoa(server_ip));
                break;
            default:
                syslog(LOG_INFO, "DHCP packet has unknown option (type=%hhu, length=%hhu)", option_type, option_length);
                break;
        }

        i += option_length + 2;
    }

    if (dhcp_message_type == -1) {
        syslog(LOG_INFO, "DHCP packet has no type");
        return;
    }

    m_state.address = packet.yiaddr;
    m_state.set_address = true;

    switch (dhcp_message_type) {
        case DHCP_MESSAGE_TYPE_OFFER:
            if (!m_state.set_default_gateway) {
                m_state.set_default_gateway = true;
                m_state.default_gateway = server_ip;
            }

            if (!send_dhcp(DHCP_MESSAGE_TYPE_REQUEST, server_ip.s_addr)) {
                syslog(LOG_WARNING, "Failed to send DHCP request packet");
            }
            break;
        case DHCP_MESSAGE_TYPE_ACK:
            m_state.flags |= IFF_UP;
            m_state.set_flags = true;
            on_configured(*this);
            syslog(LOG_INFO, "DHCP bound interface to ip `%s'", inet_ntoa(packet.yiaddr));
            break;
        default:
            syslog(LOG_INFO, "ignoring DHCP message type of `%u'", dhcp_message_type);
            break;
    }
}
