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

void Application::invalidate(const Rect& rect) {
    m_dirty_rects.add(rect);
    schedule_render();
}

void Application::render() {
    m_io_terminal->set_show_cursor(false);

    Panel::render();

    if (auto panel = focused_panel()) {
        if (auto cursor_position = panel->cursor_position()) {
            auto translated = cursor_position->translated(panel->positioned_rect().top_left());
            if (m_io_terminal->terminal_rect().intersects(translated) && panel->positioned_rect().intersects(translated)) {
                m_io_terminal->move_cursor_to(translated);
                m_io_terminal->set_show_cursor(true);
            }
        }
    }
    m_io_terminal->flush();

    m_dirty_rects.clear();
}

void Application::schedule_render() {
    if (m_render_scheduled) {
        return;
    }

    m_render_scheduled = true;
    deferred_invoke([this] {
        render();
        m_render_scheduled = false;
    });
}

void Application::on_key_event(const App::KeyEvent& event) {
    if (auto panel = focused_panel(); panel && panel.get() != this) {
        return panel->on_key_event(event);
    }
    this->make_focused();
    Panel::on_key_event(event);
}

void Application::on_mouse_event(const App::MouseEvent& event) {
    if (auto panel = focused_panel(); panel && panel.get() != this && panel->steals_focus()) {
        auto new_event = translate_mouse_event(*panel, event);
        return panel->on_mouse_event(new_event);
    }

    if (!event.mouse_down_any()) {
        if (auto panel = focused_panel(); panel && panel.get() != this) {
            auto new_event = translate_mouse_event(*panel, event);
            panel->on_mouse_event(new_event);
        }
        return;
    }

    auto* panel = hit_test(*this, { event.x(), event.y() });
    panel->make_focused();
    if (auto panel = focused_panel(); panel && panel.get() != this) {
        auto new_event = translate_mouse_event(*panel, event);
        return panel->on_mouse_event(new_event);
    }
    this->make_focused();
    Panel::on_mouse_event(event);
}

App::MouseEvent Application::translate_mouse_event(const Panel& panel, const App::MouseEvent& event) const {
    return App::MouseEvent(event.mouse_event_type(), event.buttons_down(), event.x() - panel.positioned_rect().x(),
                           event.y() - panel.positioned_rect().y(), event.z(), event.button(), event.modifiers());
}

Panel* Application::hit_test(const Panel& root, const Point& point) const {
    for (auto& child : root.children()) {
        if (child->is_panel()) {
            auto& panel = static_cast<const Panel&>(*child);
            if (auto* result = hit_test(panel, point)) {
                return result;
            }
        }
    }

    if (root.positioned_rect().intersects(point)) {
        return const_cast<Panel*>(&root);
    }
    return nullptr;
}

void Application::set_focused_panel(Panel* panel) {
    auto old_panel = m_focused_panel.lock();
    if (old_panel.get() == panel) {
        return;
    }

    if (old_panel) {
        old_panel->on_unfocused();
    }

    if (!panel) {
        m_focused_panel.reset();
        return;
    }

    m_focused_panel = panel->weak_from_this();
    if (panel) {
        panel->on_focused();
    }
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
