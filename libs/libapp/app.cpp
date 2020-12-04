#include <app/app.h>
#include <app/window.h>
#include <assert.h>
#include <eventloop/event.h>
#include <eventloop/selectable.h>
#include <liim/function.h>
#include <sys/mman.h>
#include <window_server/message.h>

namespace App {

static App* s_app;

void WindowServerClient::initialize() {
    m_server = IPC::Endpoint::create(shared_from_this());
    m_server->set_dispatcher(shared_from_this());

    auto socket = UnixSocket::create_connection(shared_from_this(), "/tmp/.window_server.socket");
    m_server->set_socket(move(socket));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowClosedEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowEvent>(WindowEvent::Type::Close));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::MouseEventMessage& message) {
    auto mouse_event_type = s_app->m_mouse_tracker.notify_mouse_event(message.left, message.right, message.x, message.y, message.scroll);
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<MouseEvent>(mouse_event_type, s_app->m_mouse_tracker.buttons_down(), message.x,
                                                                         message.y, message.scroll, message.left, message.right));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::KeyEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<KeyEvent>(message.event.ascii, message.event.key, message.event.flags));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowDidResizeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowEvent>(WindowEvent::Type::DidResize));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowStateChangeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowStateEvent>(message.active));
}

App::App() {
    assert(!s_app);
    s_app = this;
    m_palette = Palette::create_from_shared_memory("/.shared_theme", PROT_READ);
    m_client = WindowServerClient::create(nullptr);
}

App::~App() {}

App& App::the() {
    return *s_app;
}
}
