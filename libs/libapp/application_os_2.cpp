#include <app/application_os_2.h>
#include <app/window_os_2.h>

namespace App {
void WindowServerClient::initialize() {
    m_server = IPC::Endpoint::create(shared_from_this());
    m_server->set_dispatcher(shared_from_this());

    auto socket = UnixSocket::create_connection(shared_from_this(), "/tmp/.window_server.socket");
    m_server->set_socket(move(socket));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowClosedEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowCloseEvent>());
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::MouseEventMessage& message) {
    auto events =
        Application::the().input_tracker().notify_mouse_event(message.buttons, message.x, message.y, message.z, message.modifiers);
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    for (auto& event : events) {
        EventLoop::queue_event(maybe_window.value(), move(event));
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::KeyEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(),
                           Application::the().input_tracker().notify_key_event(static_cast<Key>(message.key), message.modifiers,
                                                                               message.generates_text, message.key_down));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::TextEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window);
    EventLoop::queue_event(*maybe_window, make_unique<TextEvent>(message.text));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowDidResizeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowDidResizeEvent>());
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowStateChangeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowStateEvent>(message.active));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ThemeChangeMessage&) {
    Window::for_each_window([&](auto& window) {
        EventLoop::queue_event(window.weak_from_this(), make_unique<ThemeChangeEvent>());
    });
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCreatedWindow& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_create_window(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidChangeWindowTitle& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_change_window_title(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCloseWindow& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_close_window(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidMakeWindowActive& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_make_window_active(data);
    }
}

OSApplication& OSApplication::the() {
    auto& app = Application::the();
    assert(app.is_os_application());
    return static_cast<OSApplication&>(app);
}

OSApplication::OSApplication() {
    initialize_palette(Palette::create_from_shared_memory("/.shared_theme", PROT_READ));
    m_client = WindowServerClient::create(nullptr);
}

OSApplication::~OSApplication() {}

UniquePtr<PlatformWindow> OSApplication::create_window(Window& window, int x, int y, int width, int height, String name, bool has_alpha,
                                                       WindowServer::WindowType type, wid_t parent_id) {
    return make_unique<OSWindow>(window, x, y, width, height, move(name), has_alpha, type, parent_id);
}

void OSApplication::set_active_window(wid_t id) {
    m_client->server().send<WindowServer::Client::SetActiveWindow>({ .wid = id });
}

void OSApplication::set_global_palette(const String& path) {
    m_client->server().send_then_wait<WindowServer::Client::ChangeThemeRequest, WindowServer::Server::ChangeThemeResponse>(
        { .path = path });
}

void OSApplication::set_window_server_listener(WindowServerListener& listener) {
    if (!m_client->window_server_listener()) {
        m_client->server().send<WindowServer::Client::RegisterAsWindowServerListener>({});
    }
    m_client->set_window_server_listener(&listener);
}

void OSApplication::remove_window_server_listener() {
    if (m_client->window_server_listener()) {
        m_client->server().send<WindowServer::Client::UnregisterAsWindowServerListener>({});
        m_client->set_window_server_listener(nullptr);
    }
}
}
