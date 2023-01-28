#include <app/layout_engine.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>
#include <gui/application.h>
#include <gui/context_menu.h>
#include <gui/widget.h>
#include <gui/window.h>

// #define WIDGET_DEBUG

namespace GUI {
Widget::Widget() : m_palette(Application::the().palette()) {}

void Widget::did_attach() {
    on<App::MouseDownEvent>([this](const App::MouseDownEvent& event) {
        if (!m_context_menu) {
            return false;
        }

        if (event.right_button() && !m_context_menu->visible() && sized_rect().intersects({ event.x(), event.y() })) {
            m_context_menu->show(positioned_rect().top_left().translated(event.x(), event.y()));
            return true;
        }
        return false;
    });
}

Widget::~Widget() {}

Window* Widget::typed_parent_window() {
    return static_cast<Window*>(parent_window());
}

void Widget::set_context_menu(SharedPtr<ContextMenu> menu) {
    m_context_menu = move(menu);
}

Renderer Widget::get_renderer() {
    Renderer renderer(*typed_parent_window()->pixels());
    renderer.set_bounding_rect(positioned_rect());
    return renderer;
}
}
