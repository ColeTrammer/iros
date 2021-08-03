#include <signal.h>
#include <tinput/io_terminal.h>
#include <tui/application.h>
#include <tui/panel.h>

namespace TUI {
static Application* s_the { nullptr };

SharedPtr<Application> Application::try_create() {
    auto io_terminal = TInput::IOTerminal::create(stdout);
    if (!io_terminal) {
        return nullptr;
    }

    auto ret = Application::create(nullptr, move(io_terminal));
    s_the = ret.get();
    return ret;
}

Application& Application::the() {
    assert(s_the);
    return *s_the;
}

Application::Application(UniquePtr<TInput::IOTerminal> io_terminal) : m_io_terminal(move(io_terminal)) {
    m_io_terminal->on_recieved_input = [this](auto data) {
        m_parser.stream_data(data);
        auto events = m_parser.take_events();
        for (auto& event : events) {
            App::EventLoop::queue_event(weak_from_this(), move(event));
        }
    };

    m_io_terminal->on_resize = [this](const auto& rect) {
        set_positioned_rect(rect);
    };
}

Application::~Application() {}

void Application::invalidate() {
    invalidate(m_io_terminal->terminal_rect());
}

void Application::invalidate(const Rect&) {
    schedule_render();
}

void Application::render() {
    Panel::render();
    m_io_terminal->flush();
}

void Application::schedule_render() {
    if (m_render_scheduled) {
        return;
    }

    m_render_scheduled = true;
    deferred_invoke([this] {
        m_render_scheduled = false;
        render();
    });
}

void Application::enter() {
    if (!m_use_alternate_screen_buffer) {
        m_io_terminal->detect_cursor_position();
    }

    m_io_terminal->set_bracketed_paste(true);
    if (m_use_alternate_screen_buffer) {
        m_io_terminal->set_use_alternate_screen_buffer(true);
        m_io_terminal->reset_cursor();
    }
    if (m_use_mouse) {
        m_io_terminal->set_use_mouse(true);
    }
    m_io_terminal->set_show_cursor(false);
    m_io_terminal->flush();

    set_positioned_rect(m_io_terminal->terminal_rect());
    schedule_render();
    m_loop.enter();
}
}
