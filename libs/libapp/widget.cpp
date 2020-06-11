#include <app/layout.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <window_server/connection.h>
#include <window_server/window.h>

// #define WIDGET_DEBUG

namespace App {

Widget::Widget() {}

Widget::~Widget() {}

void Widget::render() {
#ifdef WIDGET_DEBUG
    Renderer renderer(*window()->pixels());
    renderer.draw_rect(m_rect);
#endif /* WIDGET_DEBUG */

    for (auto& child : children()) {
        if (child->is_widget()) {
            auto& widget = const_cast<Widget&>(static_cast<const Widget&>(*child));
            widget.render();
        }
    }
}

void Widget::on_resize() {
    if (layout()) {
        layout()->layout();
    }
}

void Widget::set_rect(const Rect& rect) {
    int old_width = m_rect.width();
    int old_height = m_rect.height();

    m_rect = rect;

    if (old_width != rect.width() || old_height != rect.height()) {
        on_resize();
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

}
