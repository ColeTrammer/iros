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
    return static_cast<Application&>(App::Application::the());
}

Application::Application(UniquePtr<TInput::IOTerminal> io_terminal) : m_io_terminal(move(io_terminal)) {
    m_io_terminal->on_recieved_input = [this](auto data) {
        m_parser.stream_data(data);
        auto events = m_parser.take_events();
        for (auto& event : events) {
            if (App::MouseEvent::is_mouse_event(*event)) {
                auto& mouse_event = static_cast<App::MouseEvent&>(*event);
                if (auto* window = hit_test({ mouse_event.x(), mouse_event.y() })) {
                    if (mouse_event.mouse_down() || mouse_event.mouse_up()) {
                        set_active_window(window);
                    }

                    if ((mouse_event.mouse_move() && !mouse_event.buttons_down()) || mouse_event.mouse_scroll()) {
                        App::EventLoop::queue_event(window->weak_from_this(), move(event));
                        continue;
                    }
                }
            }

            if (auto window = m_active_window.lock()) {
                App::EventLoop::queue_event(window->weak_from_this(), move(event));
            }
        }
    };

    m_io_terminal->on_resize = [this](const auto& rect) {
        m_root_window->set_rect(rect);
    };
}

void Application::initialize() {
    m_root_window = Window::create(this);
    m_root_window->set_rect(m_io_terminal->terminal_rect());

    set_active_window(m_root_window.get());
}

Application::~Application() {}

void Application::set_active_window(Window* window) {
    auto old_window = m_active_window.lock();
    if (window == old_window.get()) {
        return;
    }

    if (window) {
        m_active_window = window->weak_from_this();
    } else {
        m_active_window.reset();
    }
}

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

void Application::render() {
    auto& io_terminal = TUI::Application::the().io_terminal();
    io_terminal.set_show_cursor(false);

    m_root_window->do_render();

    if (auto window = m_active_window.lock()) {
        if (auto panel = window->focused_widget()) {
            if (auto cursor_position = panel->cursor_position()) {
                auto translated = cursor_position->translated(panel->positioned_rect().top_left()).translated(window->position());
                if (io_terminal.terminal_rect().intersects(translated) && panel->positioned_rect().intersects(translated)) {
                    io_terminal.move_cursor_to(translated);
                    io_terminal.set_show_cursor(true);
                }
            }
        }
    }

    io_terminal.flush();
}

void Application::schedule_render() {
    deferred_invoke_batched(m_render_scheduled, [this] {
        render();
    });
}

Window* Application::hit_test(const Point&) {
    return &root_window();
}
}
