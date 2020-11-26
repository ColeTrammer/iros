#pragma once

#include <app/mouse_press_tracker.h>
#include <eventloop/event_loop.h>
#include <graphics/palette.h>
#include <window_server/connection.h>

namespace App {

class App {
public:
    static App& the();
    App();

    WindowServer::Connection& ws_connection() { return m_connection; }

    void enter() { return m_loop.enter(); }
    EventLoop& main_event_loop() { return m_loop; }

    SharedPtr<Palette> palette() const { return m_palette; }

private:
    void setup_ws_connection_notifier();
    void process_ws_message(UniquePtr<WindowServer::Message> message);

    EventLoop m_loop;
    WindowServer::Connection m_connection;
    MousePressTracker m_mouse_tracker;
    SharedPtr<Palette> m_palette;
};
}
