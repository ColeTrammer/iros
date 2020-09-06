#include <app/mouse_press_tracker.h>
#include <stdio.h>

namespace App {

MouseEventType MousePressTracker::notify_mouse_event(mouse_button_state left, mouse_button_state right, int x, int y, scroll_state scroll) {
    int button = 0;
    MouseEventType type = MouseEventType::Move;
    if (left == MOUSE_DOWN) {
        type = MouseEventType::Down;
        button = MouseButton::Left;
        m_buttons_down |= MouseButton::Left;
    } else if (left == MOUSE_UP) {
        type = MouseEventType::Up;
        button = MouseButton::Left;
        m_buttons_down &= ~MouseButton::Left;
    }

    if (right == MOUSE_DOWN) {
        type = MouseEventType::Down;
        button = MouseButton::Right;
        m_buttons_down |= MouseButton::Right;
    } else if (right == MOUSE_UP) {
        type = MouseEventType::Up;
        button = MouseButton::Right;
        m_buttons_down &= ~MouseButton::Right;
    }

    if (scroll != SCROLL_NONE) {
        m_prev.clear();
    }

    switch (type) {
        case MouseEventType::Down:
            break;
        default:
            return type;
    }

    return m_prev.set(button, x, y);
}

MouseEventType MousePressTracker::MousePress::set(int button, int x, int y) {
    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    auto time_delta_ms = (now.tv_sec - m_timestamp.tv_sec) * 1000 + (now.tv_nsec - m_timestamp.tv_nsec) / 1000000;
    bool is_repetition = m_button == button && m_x == x && m_y == y && time_delta_ms <= 500;

    m_x = x;
    m_y = y;
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
