#pragma once

#include <app/udp_socket.h>
#include <liim/string.h>
#include <sys/umessage.h>

#include "dhcp.h"

class Interface {
public:
    static SharedPtr<Interface> create(const umessage_interface_desc& desc, uint32_t dhcp_xid);
    Interface(const umessage_interface_desc& desc, uint32_t dhcp_xid, SharedPtr<App::UdpSocket> socket);

    bool send_dhcp(uint8_t message_type, in_addr_t server_ip = INADDR_NONE);
    void handle_dhcp_response(const dhcp_packet& packet);

    const umessage_interface_set_state_request& state() const { return m_state; }
    const String& name() const { return m_name; }

    Function<void(Interface&)> on_configured;

private:
    link_layer_address m_link_layer_address;
    umessage_interface_set_state_request m_state;
    SharedPtr<App::UdpSocket> m_dhcp_socket;
    String m_name;
    uint32_t m_dhcp_xid { 0 };
    bool m_recieved_offer { false };
};
