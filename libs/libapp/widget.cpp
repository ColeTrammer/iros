#include <app/layout.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <window_server/connection.h>
#include <window_server/window.h>

namespace App {

Widget::Widget() {}

Widget::~Widget() {}

void Widget::render() {
    Renderer renderer(*window()->pixels());
    renderer.draw_rect(m_rect);
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
