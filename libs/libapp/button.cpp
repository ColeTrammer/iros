#include <app/button.h>
#include <app/event.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void Button::render() {
    Renderer renderer(*window()->pixels());
    renderer.draw_rect(rect());
    renderer.render_text(rect().x() + 2, rect().y() + 2, label());
}

void Button::on_mouse_event(MouseEvent& mouse_event) {
    if (mouse_event.left() == MOUSE_DOWN) {
        m_did_mousedown = true;
        return;
    }

    if (m_did_mousedown && mouse_event.left() == MOUSE_UP) {
        m_did_mousedown = false;
        if (on_click) {
            on_click();
        }
    }
}

}
