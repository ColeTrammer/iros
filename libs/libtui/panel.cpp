#include <eventloop/event.h>
#include <eventloop/key_bindings.h>
#include <liim/maybe.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/layout_engine.h>
#include <tui/panel.h>

namespace TUI {
Panel::Panel() {}

void Panel::initialize() {
    on<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        if (event.control_down() && event.key() == App::Key::Q) {
            Application::the().event_loop().set_should_exit(true);
            return true;
        }
        return false;
    });

    on<App::ResizeEvent>([this](const App::ResizeEvent&) {
        if (m_layout_engine) {
            m_layout_engine->layout();
        }
        return false;
    });

    Object::initialize();
}

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

    dispatch(App::ResizeEvent {});
}

void Panel::invalidate() {
    invalidate(sized_rect());
}

void Panel::invalidate(const Rect& rect) {
    Application::the().invalidate(rect.translated(positioned_rect().top_left()));
}

void Panel::remove() {
    TUI::Application::the().invalidate(positioned_rect());
    if (parent()) {
        auto* panel_with_focus = focus_proxy() ? focus_proxy() : (accepts_focus() ? this : nullptr);
        if (panel_with_focus && TUI::Application::the().focused_panel().get() == panel_with_focus) {
            if (auto* parent = parent_panel()) {
                parent->make_focused();
            }
        }
        parent()->remove_child(shared_from_this());
    }
}

Panel* Panel::parent_panel() {
    if (!parent()) {
        return nullptr;
    }
    if (!parent()->is_panel()) {
        return nullptr;
    }
    return static_cast<Panel*>(parent());
}

void Panel::make_focused() {
    auto& panel_to_check = focus_proxy() ? *focus_proxy() : *this;
    if (!panel_to_check.accepts_focus()) {
        if (auto* parent = parent_panel()) {
            return parent->make_focused();
        }
        return;
    }
    TUI::Application::the().set_focused_panel(&panel_to_check);
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
}
