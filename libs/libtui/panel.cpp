#include <eventloop/event.h>
#include <liim/maybe.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/layout_engine.h>
#include <tui/panel.h>

namespace TUI {
Panel::Panel() {}

Panel::~Panel() {}

void Panel::do_set_layout_engine(UniquePtr<LayoutEngine> engine) {
    m_layout_engine = move(engine);
}

void Panel::set_positioned_rect(const Rect& rect) {
    if (m_positioned_rect == rect) {
        return;
    }

    invalidate();
    m_positioned_rect = rect;
    invalidate();

    on_resize();
}

void Panel::invalidate() {
    invalidate(sized_rect());
}

void Panel::invalidate(const Rect& rect) {
    Application::the().invalidate(rect.translated(positioned_rect().top_left()));
}

void Panel::on_resize() {
    if (m_layout_engine) {
        m_layout_engine->layout();
    }
}

Maybe<Point> Panel::cursor_position() {
    return {};
}

TInput::TerminalRenderer Panel::get_renderer() {
    auto dirty_rects = TUI::Application::the().dirty_rects();
    Function<void(Panel&)> enumerate_children = [&](Panel& panel) {
        for (auto& child : panel.children()) {
            if (child->is_panel()) {
                auto& panel = const_cast<Panel&>(static_cast<const Panel&>(*child));
                dirty_rects.subtract(panel.positioned_rect());
                enumerate_children(panel);
            }
        }
    };
    enumerate_children(*this);

    auto renderer = TInput::TerminalRenderer { Application::the().io_terminal(), dirty_rects };
    renderer.set_clip_rect(positioned_rect());
    renderer.set_origin(positioned_rect().top_left());
    return renderer;
}

void Panel::render() {
    for (auto& child : children()) {
        if (child->is_panel()) {
            auto& panel = const_cast<Panel&>(static_cast<const Panel&>(*child));
            if (Application::the().dirty_rects().intersects(panel.positioned_rect())) {
                panel.render();
            }
        }
    }
}

void Panel::on_mouse_down(const App::MouseEvent&) {}

void Panel::on_mouse_double(const App::MouseEvent& event) {
    return on_mouse_down(event);
}

void Panel::on_mouse_triple(const App::MouseEvent& event) {
    return on_mouse_down(event);
}

void Panel::on_mouse_up(const App::MouseEvent&) {}

void Panel::on_mouse_move(const App::MouseEvent&) {}

void Panel::on_mouse_scroll(const App::MouseEvent&) {}

void Panel::on_key_event(const App::KeyEvent& event) {
    if (event.control_down() && event.key() == App::Key::Q) {
        Application::the().event_loop().set_should_exit(true);
    }
}

void Panel::on_mouse_event(const App::MouseEvent& event) {
    switch (event.mouse_event_type()) {
        case App::MouseEventType::Down:
            return on_mouse_down(event);
        case App::MouseEventType::Double:
            return on_mouse_double(event);
        case App::MouseEventType::Triple:
            return on_mouse_triple(event);
        case App::MouseEventType::Up:
            return on_mouse_up(event);
        case App::MouseEventType::Move:
            return on_mouse_move(event);
        case App::MouseEventType::Scroll:
            return on_mouse_scroll(event);
    }
}

void Panel::on_event(const App::Event& event) {
    switch (event.type()) {
        case App::Event::Type::Mouse:
            return on_mouse_event(static_cast<const App::MouseEvent&>(event));
        case App::Event::Type::Key:
            return on_key_event(static_cast<const App::KeyEvent&>(event));
        default:
            return;
    }
}
}
