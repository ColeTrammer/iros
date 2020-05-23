#pragma once

#include <app/event_loop.h>
#include <window_server/connection.h>

namespace App {

class App {
public:
    static App& the();
    App();

    WindowServer::Connection& ws_connection() { return m_connection; }

    void enter() { return m_loop.enter(); }

private:
    void setup_ws_connection_notifier();
    void process_ws_message(UniquePtr<WindowServer::Message> message);

    EventLoop m_loop;
    WindowServer::Connection m_connection;
};

}
