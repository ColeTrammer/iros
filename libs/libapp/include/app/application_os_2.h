#pragma once

#include <app/application.h>

namespace App {
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

class OSApplication final : public Application {
public:
    static OSApplication& the();

    OSApplication();
    virtual ~OSApplication() override;

    WindowServerClient& ws() { return *m_client; }

    virtual void set_window_server_listener(WindowServerListener& listener) override;
    virtual void remove_window_server_listener() override;

    virtual void set_active_window(wid_t id) override;
    virtual void set_global_palette(const String& path) override;

private:
    virtual bool is_os_application() const override { return true; }

    SharedPtr<WindowServerClient> m_client;
};
}
