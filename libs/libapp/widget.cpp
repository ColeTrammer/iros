#include <app/application.h>
#include <app/context_menu.h>
#include <app/layout_engine.h>
#include <app/widget.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

// #define WIDGET_DEBUG

namespace App {
Widget::Widget() : m_palette(Application::the().palette()) {}

void Widget::initialize() {
    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        if (!m_context_menu) {
            return false;
        }

        if (event.right_button() && !m_context_menu->visible() && sized_rect().intersects({ event.x(), event.y() })) {
            m_context_menu->show(positioned_rect().top_left().translated(event.x(), event.y()));
            return true;
        }
        return false;
    });

    Base::Widget::initialize();
}

Widget::~Widget() {}

Window* Widget::parent_window() {
    return static_cast<Window*>(Base::Widget::parent_window());
}

void Widget::set_context_menu(SharedPtr<ContextMenu> menu) {
    m_context_menu = move(menu);
}

Renderer Widget::get_renderer() {
    Renderer renderer(*parent_window()->pixels());
    renderer.set_bounding_rect(positioned_rect());
    return renderer;
}
}
