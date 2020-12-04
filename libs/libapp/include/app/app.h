#pragma once

#include <app/mouse_press_tracker.h>
#include <eventloop/event_loop.h>
#include <graphics/palette.h>
#include <window_server/message.h>

namespace App {

class WindowServerClient final : public WindowServer::Server::MessageDispatcher {
    APP_OBJECT(WindowServerClient)

public:
    virtual void initialize() override;

    IPC::Endpoint& server() { return *m_server; }

private:
    virtual void handle_error(IPC::Endpoint&) override { assert(false); }

    virtual void handle(IPC::Endpoint&, const WindowServer::Server::CreateWindowResponse&) override {}
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::RemoveWindowResponse&) override {}
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ChangeWindowVisibilityResponse&) override {}
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::KeyEventMessage&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::MouseEventMessage&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::WindowDidResizeMessage&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::WindowReadyToResizeResponse&) override {}
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::WindowClosedEventMessage&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::WindowStateChangeMessage&) override;

    SharedPtr<IPC::Endpoint> m_server;
};

class App {
public:
    static App& the();
    App();
    ~App();

    void enter() { return m_loop.enter(); }
    EventLoop& main_event_loop() { return m_loop; }

    SharedPtr<Palette> palette() const { return m_palette; }

    WindowServerClient& ws() { return *m_client; }

private:
    friend class WindowServerClient;

    EventLoop m_loop;
    MousePressTracker m_mouse_tracker;
    SharedPtr<Palette> m_palette;
    SharedPtr<WindowServerClient> m_client;
};
}
