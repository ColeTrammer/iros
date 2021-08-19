#pragma once

#include <eventloop/event.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <time.h>

namespace App {
class InputTracker {
public:
    Vector<UniquePtr<MouseEvent>> notify_mouse_event(int buttons, int x, int y, int z, int modifiers);

    Vector<UniquePtr<MouseEvent>> notify_os_mouse_event(int scale_mode, int dx, int dy, int dz, int buttons, int screen_width,
                                                        int screen_height);
    UniquePtr<KeyEvent> notify_os_key_event(char ascii, int key, unsigned int flags);

    int prev_buttons() const { return m_prev.buttons_down(); }
    int prev_x() const { return m_prev.x(); }
    int prev_y() const { return m_prev.y(); }

private:
    class MousePress {
    public:
        StringView set(int x, int y, int button);
        void clear() {
            m_button = 0;
            m_double = false;
        }

        const timespec& timestamp() const { return m_timestamp; }
        int x() const { return m_x; }
        int y() const { return m_y; }
        int button() const { return m_button; }
        int buttons_down() const { return m_buttons_down; }
        bool double_click() const { return m_double; }

        void set_x(int x) { m_x = x; }
        void set_y(int y) { m_y = y; }
        void set_buttons_down(int buttons_down) { m_buttons_down = buttons_down; }

    private:
        timespec m_timestamp;
        int m_x { 0 };
        int m_y { 0 };
        int m_button { 0 };
        int m_buttons_down { 0 };
        bool m_double { false };
    };

    MousePress m_prev;
};
}
