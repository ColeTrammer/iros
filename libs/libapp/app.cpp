#include <app/app.h>
#include <app/selectable.h>
#include <assert.h>
#include <liim/function.h>
#include <window_server/message.h>

namespace App {

static App* s_app;

void App::setup_ws_connection_notifier() {
    class FdWrapper final : public Selectable {
    public:
        FdWrapper(int fd) { set_fd(fd); }

        Function<void()> on_readable;

        virtual void notify_readable() override {
            if (on_readable) {
                on_readable();
            }
        }
    };

    static FdWrapper* fd_wrapper = new FdWrapper(ws_connection().fd());
    fd_wrapper->set_selected_events(NotifyWhen::Readable);
    fd_wrapper->on_readable = [&] {
        auto message = m_connection.recieve_message();
        if (message) {
            process_ws_message(move(message));
        }
    };

    EventLoop::register_selectable(*fd_wrapper);
}

void App::process_ws_message(UniquePtr<WindowServer::Message>) {}

App::App() {
    assert(!s_app);
    s_app = this;
    setup_ws_connection_notifier();
}

App& App::the() {
    return *s_app;
}

}
