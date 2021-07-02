#include <app/application.h>
#include <app/context_menu.h>
#include <app/layout.h>
#include <app/widget.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

// #define WIDGET_DEBUG

namespace App {
Widget::Widget() : m_palette(Application::the().palette()) {}

Widget::~Widget() {}

void Widget::render() {
#ifdef WIDGET_DEBUG
    auto renderer = get_renderer();
    renderer.draw_rect(sized_rect(), outline_color());
#endif /* WIDGET_DEBUG */

    for (auto& child : children()) {
        if (child->is_widget()) {
            auto& widget = const_cast<Widget&>(static_cast<const Widget&>(*child));
            if (!widget.hidden()) {
                widget.render();
            }
        }
    }
}

void Widget::on_resize() {
    if (layout()) {
        layout()->layout();
    }
}

void Widget::on_theme_change_event(const ThemeChangeEvent& event) {
    for (auto& child : children()) {
        if (child->is_widget()) {
            auto& widget = const_cast<Widget&>(static_cast<const Widget&>(*child));
            widget.on_theme_change_event(event);
        }
    }
}

void Widget::set_positioned_rect(const Rect& rect) {
    int old_width = m_positioned_rect.width();
    int old_height = m_positioned_rect.height();

    m_positioned_rect = rect;

    if (old_width != rect.width() || old_height != rect.height()) {
        on_resize();
    }
}

void Widget::set_preferred_size(const Size& size) {
    if (m_preferred_size.width == size.width && m_preferred_size.height == size.height) {
        return;
    }

    m_preferred_size = size;
    if (auto* parent = this->parent()) {
        if (parent->is_widget()) {
            if (auto* parent_layout = static_cast<Widget&>(*parent).layout()) {
                parent_layout->layout();
            }
        }
    }
}

Window* Widget::window() {
    Object* object = parent();
    while (object) {
        if (object->is_window()) {
            return static_cast<Window*>(object);
        }
        object = object->parent();
    }
    return nullptr;
}

void Widget::invalidate(const Rect& rect) {
    window()->invalidate_rect(rect);
}

void Widget::set_context_menu(SharedPtr<ContextMenu> menu) {
    m_context_menu = move(menu);
}

void Widget::on_mouse_event(const MouseEvent& event) {
    switch (event.mouse_event_type()) {
        case MouseEventType::Down:
            return on_mouse_down(event);
        case MouseEventType::Double:
            return on_mouse_double(event);
        case MouseEventType::Triple:
            return on_mouse_triple(event);
        case MouseEventType::Up:
            return on_mouse_up(event);
        case MouseEventType::Move:
            return on_mouse_move(event);
        case MouseEventType::Scroll:
            return on_mouse_scroll(event);
    }
}

void Widget::on_mouse_down(const MouseEvent& event) {
    if (!m_context_menu) {
        return;
    }

    if (event.right_button() && !m_context_menu->visible() && sized_rect().intersects({ event.x(), event.y() })) {
        m_context_menu->show(positioned_rect().top_left().translated(event.x(), event.y()));
    }
}

void Widget::on_mouse_double(const MouseEvent& ev) {
    return on_mouse_down(ev);
}

void Widget::on_mouse_triple(const MouseEvent& ev) {
    return on_mouse_down(ev);
}

void Widget::on_mouse_up(const MouseEvent&) {}

void Widget::on_mouse_move(const MouseEvent&) {}

void Widget::on_mouse_scroll(const MouseEvent&) {}

void Widget::set_hidden(bool b) {
    if (m_hidden == b) {
        return;
    }

    m_hidden = b;
    invalidate(positioned_rect());
}

Renderer Widget::get_renderer() {
    Renderer renderer(*window()->pixels());
    renderer.set_bounding_rect(positioned_rect());
    return renderer;
}
}
