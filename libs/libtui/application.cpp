#include <app/layout_engine.h>
#include <liim/format.h>
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
    return static_cast<Application&>(App::Base::Application::the());
}

Application::Application(UniquePtr<TInput::IOTerminal> io_terminal) : m_io_terminal(move(io_terminal)) {
    m_io_terminal->on_recieved_input = [this](auto data) {
        m_parser.stream_data(data);
        auto events = m_parser.take_events();
        for (auto& event : events) {
            App::EventLoop::queue_event(m_root_window->weak_from_this(), move(event));
        }
    };

    m_io_terminal->on_resize = [this](const auto& rect) {
        m_root_window->set_rect(rect);
    };
}

void Application::initialize() {
    m_root_window = RootWindow::create(shared_from_this());
    m_root_window->set_rect(m_io_terminal->terminal_rect());
}

Application::~Application() {}

App::Margins Application::default_margins() const {
    return {};
}

int Application::default_spacing() const {
    return 0;
}

void Application::before_enter() {
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
}
}
