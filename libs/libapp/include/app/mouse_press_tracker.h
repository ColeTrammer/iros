#pragma once

#include <eventloop/event.h>
#include <time.h>

namespace App {

class MousePressTracker {
public:
    MouseEventType notify_mouse_event(mouse_button_state left, mouse_button_state right, int x, int y, scroll_state scroll);

    int buttons_down() const { return m_buttons_down; }

private:
    class MousePress {
    public:
        MouseEventType set(int button, int x, int y);
        void clear() { m_button = 0; }

        const timespec& timestamp() const { return m_timestamp; }
        int x() const { return m_x; }
        int y() const { return m_y; }
        int button() const { return m_button; }
        bool double_click() const { return m_double; }

    private:
        timespec m_timestamp;
        int m_x { 0 };
        int m_y { 0 };
        int m_button { 0 };
        bool m_double { false };
    };

    MousePress m_prev;
    int m_buttons_down { 0 };
};

}
