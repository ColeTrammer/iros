#include <eventloop/event.h>
#include <graphics/renderer.h>
#include <gui/button.h>
#include <gui/window.h>

namespace GUI {
Button::Button(String label) : m_label(move(label)) {}

void Button::did_attach() {
    set_accepts_focus(true);

    on<App::MouseDownEvent>([this](const App::MouseDownEvent& event) {
        if (event.left_button()) {
            m_did_mousedown = true;
            return true;
        }
        return false;
    });

    on<App::MouseUpEvent>([this](const App::MouseUpEvent& event) {
        if (m_did_mousedown && event.left_button()) {
            m_did_mousedown = false;
            emit<ClickEvent>();
            return true;
        }
        return false;
    });

    return Widget::did_attach();
}

void Button::render() {
    auto renderer = get_renderer();

    renderer.fill_rect(sized_rect(), background_color());
    renderer.draw_rect(sized_rect(), outline_color());
    renderer.render_text(label(), sized_rect().adjusted(-2), text_color(), TextAlign::CenterLeft, *font());
}
}
