#include <app/app.h>
#include <app/event.h>
#include <app/selectable.h>
#include <app/window.h>
#include <assert.h>
#include <liim/function.h>
#include <window_server/message.h>

namespace App {

static App* s_app;

void App::setup_ws_connection_notifier() {
    static FdWrapper* fd_wrapper = new FdWrapper(ws_connection().fd());
    fd_wrapper->set_selected_events(NotifyWhen::Readable);
    fd_wrapper->on_readable = [this] {
        auto message = m_connection.recieve_message();
        if (message) {
            process_ws_message(move(message));
        }
    };

    EventLoop::register_selectable(*fd_wrapper);
}

void App::process_ws_message(UniquePtr<WindowServer::Message> message) {
    switch (message->type) {
        case WindowServer::Message::Type::WindowClosedEventMessage: {
            auto maybe_window = Window::find_by_wid(message->data.window_closed_event_messasge.wid);
            assert(maybe_window.has_value());
            m_loop.queue_event(maybe_window.value(), move(make_unique<WindowEvent>(WindowEvent::Type::Close)));
            break;
        }
        case WindowServer::Message::Type::MouseEventMessage: {
            auto& mouse_event = message->data.mouse_event_message;
            auto maybe_window = Window::find_by_wid(mouse_event.wid);
            assert(maybe_window.has_value());
            m_loop.queue_event(maybe_window.value(), move(make_unique<MouseEvent>(mouse_event.x, mouse_event.y, mouse_event.scroll,
                                                                                  mouse_event.left, mouse_event.right)));
            break;
        }
        case WindowServer::Message::Type::KeyEventMessage: {
            auto& key_event = message->data.key_event_message;
            auto maybe_window = Window::find_by_wid(key_event.wid);
            assert(maybe_window.has_value());
            m_loop.queue_event(maybe_window.value(),
                               move(make_unique<KeyEvent>(key_event.event.ascii, key_event.event.key, key_event.event.flags)));
            break;
        }
        case WindowServer::Message::Type::WindowDidResizeMessage: {
            auto& resize_event = message->data.window_did_resize_message;
            auto maybe_window = Window::find_by_wid(resize_event.wid);
            assert(maybe_window.has_value());
            m_loop.queue_event(maybe_window.value(), move(make_unique<WindowEvent>(WindowEvent::Type::DidResize)));
            break;
        }
        default:
            break;
    }
}

App::App() {
    assert(!s_app);
    s_app = this;
    setup_ws_connection_notifier();
}

App& App::the() {
    return *s_app;
}
}
