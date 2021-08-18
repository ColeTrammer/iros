#include <eventloop/base_widget.h>
#include <eventloop/event.h>
#include <eventloop/key_bindings.h>

namespace App {
BaseWidget::BaseWidget() {}

BaseWidget::~BaseWidget() {}

void BaseWidget::set_key_bindings(UniquePtr<KeyBindings> bindings) {
    m_key_bindings = move(bindings);
}

bool BaseWidget::handle_as_key_shortcut(const KeyEvent& event) {
    if (!m_key_bindings) {
        return false;
    }
    return m_key_bindings->handle_key_event(event);
}

void BaseWidget::on_mouse_event(const MouseEvent& event) {
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

void BaseWidget::on_key_event(const KeyEvent& event) {
    switch (event.key_event_type()) {
        case KeyEventType::Down:
            return on_key_down(event);
        case KeyEventType::Up:
            return on_key_up(event);
    }
}

void BaseWidget::on_mouse_double(const MouseEvent& ev) {
    return on_mouse_down(ev);
}

void BaseWidget::on_mouse_triple(const MouseEvent& ev) {
    return on_mouse_down(ev);
}

void BaseWidget::on_event(const Event& event) {
    switch (event.type()) {
        case App::Event::Type::Mouse:
            return on_mouse_event(static_cast<const App::MouseEvent&>(event));
        case App::Event::Type::Text:
            return on_text_event(static_cast<const App::TextEvent&>(event));
        case App::Event::Type::Key:
            return on_key_event(static_cast<const App::KeyEvent&>(event));
        default:
            break;
    }
}
}
