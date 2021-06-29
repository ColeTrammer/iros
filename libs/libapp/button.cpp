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

void Button::on_mouse_event(MouseEvent& mouse_event) {
    if (mouse_event.mouse_event_type() == App::MouseEventType::Down && mouse_event.button() == App::MouseButton::Left) {
        m_did_mousedown = true;
        return;
    }

    if (m_did_mousedown && mouse_event.mouse_event_type() == App::MouseEventType::Up && mouse_event.button() == App::MouseButton::Left) {
        m_did_mousedown = false;
        if (on_click) {
            on_click();
        }
    }
}
}
