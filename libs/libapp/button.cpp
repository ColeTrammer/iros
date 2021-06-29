#include <app/button.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

namespace App {

void Button::render() {
    Renderer renderer(*window()->pixels());

    renderer.fill_rect(positioned_rect(), background_color());
    renderer.draw_rect(positioned_rect(), outline_color());
    renderer.render_text(label(), positioned_rect().adjusted(-2), text_color(), TextAlign::CenterLeft, font());
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
