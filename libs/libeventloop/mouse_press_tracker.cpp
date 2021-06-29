#include <eventloop/mouse_press_tracker.h>
#include <stdio.h>

namespace App {
Vector<UniquePtr<MouseEvent>> MousePressTracker::notify_mouse_event(int buttons, int x, int y, int z) {
    Vector<UniquePtr<MouseEvent>> events;
    if (z != 0) {
        events.add(make_unique<MouseEvent>(MouseEventType::Scroll, buttons, x, y, z, MouseButton::None));
    }

    if (m_prev.x() != x || m_prev.y() != y) {
        events.add(make_unique<MouseEvent>(MouseEventType::Move, m_prev.buttons_down(), x, y, 0, MouseButton::None));
    }

    auto buttons_to_pass = m_prev.buttons_down();
    auto handle_button = [&](int button) {
        if (!(buttons & button) && !!(m_prev.buttons_down() & button)) {
            buttons_to_pass &= ~button;
            events.add(make_unique<MouseEvent>(MouseEventType::Up, buttons_to_pass, x, y, 0, button));
        }

        if (!!(buttons & button) && !(m_prev.buttons_down() & button)) {
            auto type = m_prev.set(x, y, button);
            buttons_to_pass |= button;
            events.add(make_unique<MouseEvent>(type, buttons_to_pass, x, y, 0, button));
        }
    };

    handle_button(MouseButton::Left);
    handle_button(MouseButton::Right);
    handle_button(MouseButton::Middle);

    m_prev.set_x(x);
    m_prev.set_y(y);
    m_prev.set_buttons_down(buttons);

    return events;
}

MouseEventType MousePressTracker::MousePress::set(int x, int y, int button) {
    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    auto time_delta_ms = (now.tv_sec - m_timestamp.tv_sec) * 1000 + (now.tv_nsec - m_timestamp.tv_nsec) / 1000000;
    bool is_repetition = m_button == button && m_x == x && m_y == y && time_delta_ms <= 500;

    m_button = button;
    m_timestamp = now;

    if (is_repetition) {
        if (m_double) {
            m_double = false;
            return MouseEventType::Triple;
        }
        m_double = true;
        return MouseEventType::Double;
    }
    m_double = false;
    return MouseEventType::Down;
}
}
