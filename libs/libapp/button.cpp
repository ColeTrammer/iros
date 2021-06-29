#include <app/button.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

namespace App {
void Button::render() {
    auto renderer = get_renderer();

    renderer.fill_rect(sized_rect(), background_color());
    renderer.draw_rect(sized_rect(), outline_color());
    renderer.render_text(label(), sized_rect().adjusted(-2), text_color(), TextAlign::CenterLeft, font());
}

void Button::on_mouse_down(const MouseEvent& mouse_event) {
    if (mouse_event.left_button()) {
        m_did_mousedown = true;
        return;
    }
}

void Button::on_mouse_up(const MouseEvent& mouse_event) {
    if (m_did_mousedown && mouse_event.left_button()) {
        m_did_mousedown = false;
        if (on_click) {
            on_click();
        }
    }
}
}
