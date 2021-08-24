#include <app/base/window.h>
#include <eventloop/event.h>

namespace App::Base {
template<typename MouseEventType>
static MouseEventType translate_mouse_event(const Widget& widget, const MouseEventType& event) {
    return MouseEventType {
        event.buttons_down(), event.x() - widget.positioned_rect().x(), event.y() - widget.positioned_rect().y(), event.z(), event.button(),
        event.modifiers()
    };
}

Window::Window() {}

void Window::initialize() {
    on<MouseDownEvent, MouseDoubleEvent, MouseTripleEvent, MouseMoveEvent, MouseUpEvent, MouseScrollEvent>([this](const auto& event) {
        // Rules for MouseEvents forwarding:
        // 1. If the currently focused widget steals focus, it gets the event no matter what.
        // 2. If the event is a MouseMove/MouseScroll events and no buttons are down, then the hovered
        //    widget gets updated (which generates Leave/Enter Events).
        // 3. If the event is a MouseDown event, then the Widget being clicked on gets the
        //    event and is focused (which generates Focus/Unfocused Events)
        // 4. Otherwise, the event is a MouseUp or MouseMove with buttons down. The currently
        //    focused widget gets the event, and the hit test result is ignored.
        if (auto widget = focused_widget(); widget->steals_focus()) {
            return forward_to(*widget, translate_mouse_event(*widget, event));
        }

        auto* widget = hit_test(main_widget(), { event.x(), event.y() });
        if (!event.mouse_down_any() && !event.mouse_up() && !event.buttons_down()) {
            set_hovered_widget(widget);
            if (widget) {
                return forward_to(*widget, translate_mouse_event(*widget, event));
            }
            return false;
        }

        if (event.mouse_down_any()) {
            set_focused_widget(widget);
        } else {
            widget = focused_widget().get();
        }
        if (widget) {
            return forward_to(*widget, translate_mouse_event(*widget, event));
        }
        return false;
    });

    on<KeyDownEvent, KeyUpEvent, TextEvent>([this](const Event& event) {
        if (auto widget = focused_widget()) {
            return forward_to(*widget, event);
        }
        return false;
    });

    Object::initialize();
}

Window::~Window() {}

void Window::set_rect(const Rect& rect) {
    if (m_rect == rect) {
        return;
    }

    m_rect = rect;
    if (m_main_widget) {
        m_main_widget->set_positioned_rect(rect);
    }
}

void Window::invalidate_rect(const Rect& rect) {
    m_dirty_rects.add(rect);
    schedule_render();
}

void Window::set_focused_widget(Widget* widget) {
    auto old_widget = m_focused_widget.lock();
    if (old_widget.get() == widget) {
        return;
    }

    if (old_widget) {
        old_widget->emit<App::UnfocusedEvent>();
    }

    if (!widget) {
        m_focused_widget.reset();
        return;
    }

    m_focused_widget = widget->weak_from_this();
    widget->emit<App::FocusedEvent>();
}

SharedPtr<Widget> Window::focused_widget() {
    auto widget = m_focused_widget.lock();
    if (!widget) {
        m_focused_widget.reset();
    }
    return widget;
}

Widget* Window::hit_test(const Widget& root, const Point& point) const {
    for (auto& child : root.children()) {
        if (child->is_base_widget()) {
            auto& widget_child = const_cast<Widget&>(static_cast<const Widget&>(*child));
            if (auto* result = hit_test(widget_child, point)) {
                return result;
            }
        }
    }
    if (!root.hidden() && root.positioned_rect().intersects(point)) {
        return const_cast<Widget*>(&root);
    }
    return nullptr;
}

void Window::schedule_render() {
    deferred_invoke_batched(m_render_scheduled, [this] {
        do_render();
    });
}

void Window::set_hovered_widget(Widget* widget) {
    auto old_widget = m_focused_widget.lock();
    if (old_widget.get() == widget) {
        return;
    }

    if (old_widget) {
        old_widget->emit<App::LeaveEvent>();
    }

    if (!widget) {
        m_focused_widget.reset();
        return;
    }

    m_focused_widget = widget->weak_from_this();
    widget->emit<App::EnterEvent>();
}
}
