#pragma once

#include <app/mouse_press_tracker.h>
#include <eventloop/event_loop.h>
#include <graphics/palette.h>
#include <window_server/message.h>

namespace App {
class WindowServerListener {
public:
    virtual ~WindowServerListener() {}

    virtual void server_did_create_window(const WindowServer::Server::ServerDidCreatedWindow&) {}
    virtual void server_did_change_window_title(const WindowServer::Server::ServerDidChangeWindowTitle&) {}
    virtual void server_did_close_window(const WindowServer::Server::ServerDidCloseWindow&) {}
    virtual void server_did_make_window_active(const WindowServer::Server::ServerDidMakeWindowActive&) {}
};

class WindowServerClient final : public WindowServer::Server::MessageDispatcher {
    APP_OBJECT(WindowServerClient)

public:
    virtual void initialize() override;

    IPC::Endpoint& server() { return *m_server; }

    WindowServerListener* window_server_listener() { return m_window_server_listener; }
    void set_window_server_listener(WindowServerListener* listener) { m_window_server_listener = listener; }

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
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ChangeThemeResponse&) override {}
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ThemeChangeMessage&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCreatedWindow&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ServerDidChangeWindowTitle&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCloseWindow&) override;
    virtual void handle(IPC::Endpoint&, const WindowServer::Server::ServerDidMakeWindowActive&) override;

    SharedPtr<IPC::Endpoint> m_server;
    WindowServerListener* m_window_server_listener { nullptr };
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

    void set_window_server_listener(WindowServerListener& listener);
    void remove_window_server_listener();

private:
    friend class WindowServerClient;

    EventLoop m_loop;
    MousePressTracker m_mouse_tracker;
    SharedPtr<Palette> m_palette;
    SharedPtr<WindowServerClient> m_client;
};
}
