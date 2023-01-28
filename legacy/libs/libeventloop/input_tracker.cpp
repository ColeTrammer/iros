#include <eventloop/input_tracker.h>
#include <stdio.h>
#include <kernel/hal/input.h>

namespace App {
UniquePtr<KeyEvent> InputTracker::notify_os_key_event(char ascii, int key, unsigned int flags) {
    bool out_is_down = !!(flags & KEY_DOWN);

    int out_modifiers = 0;
    if (flags & KEY_CONTROL_ON) {
        out_modifiers |= KeyModifier::Control;
    }
    if (flags & KEY_SHIFT_ON) {
        out_modifiers |= KeyModifier::Shift;
    }
    if (flags & KEY_ALT_ON) {
        out_modifiers |= KeyModifier::Alt;
    }

    auto out_generates_text = [&] {
        if (!ascii) {
            return false;
        }
        if ((out_modifiers & KeyModifier::Control) || (out_modifiers & KeyModifier::Alt)) {
            return false;
        }
        return true;
    }();

    auto out_key = [&] {
        switch (key) {
            case KEY_A:
                return Key::A;
            case KEY_B:
                return Key::B;
            case KEY_C:
                return Key::C;
            case KEY_D:
                return Key::D;
            case KEY_E:
                return Key::E;
            case KEY_F:
                return Key::F;
            case KEY_G:
                return Key::G;
            case KEY_H:
                return Key::H;
            case KEY_I:
                return Key::I;
            case KEY_J:
                return Key::J;
            case KEY_K:
                return Key::K;
            case KEY_L:
                return Key::L;
            case KEY_M:
                return Key::M;
            case KEY_N:
                return Key::N;
            case KEY_O:
                return Key::O;
            case KEY_P:
                return Key::P;
            case KEY_Q:
                return Key::Q;
            case KEY_R:
                return Key::R;
            case KEY_S:
                return Key::S;
            case KEY_T:
                return Key::T;
            case KEY_U:
                return Key::U;
            case KEY_V:
                return Key::V;
            case KEY_W:
                return Key::W;
            case KEY_X:
                return Key::X;
            case KEY_Y:
                return Key::Y;
            case KEY_Z:
                return Key::Z;
            case KEY_1:
                return Key::_1;
            case KEY_2:
                return Key::_2;
            case KEY_3:
                return Key::_3;
            case KEY_4:
                return Key::_4;
            case KEY_5:
                return Key::_5;
            case KEY_6:
                return Key::_6;
            case KEY_7:
                return Key::_7;
            case KEY_8:
                return Key::_8;
            case KEY_9:
                return Key::_9;
            case KEY_0:
                return Key::_0;
            case KEY_ENTER:
                return Key::Enter;
            case KEY_ESC:
                return Key::Escape;
            case KEY_BACKSPACE:
                return Key::Backspace;
            case KEY_TAB:
                return Key::Tab;
            case KEY_SPACE:
                return Key::Space;
            case KEY_MINUS:
                return Key::Minus;
            case KEY_EQUALS:
                return Key::Equals;
            case KEY_LEFT_SQUARE_BRACKET:
                return Key::LeftBracket;
            case KEY_RIGHT_SQUARE_BRACKET:
                return Key::RightBracket;
            case KEY_BACK_SLASH:
                return Key::Backslash;
            case KEY_SEMICOLON:
                return Key::SemiColon;
            case KEY_SINGLE_QUOTE:
                return Key::Quote;
            case KEY_BACK_TICK:
                return Key::Backtick;
            case KEY_COMMA:
                return Key::Comma;
            case KEY_DOT:
                return Key::Period;
            case KEY_FORWARD_SLASH:
                return Key::Slash;
            case KEY_CAPSLOCK:
                return Key::CapsLock;
            case KEY_F1:
                return Key::F1;
            case KEY_F2:
                return Key::F2;
            case KEY_F3:
                return Key::F3;
            case KEY_F4:
                return Key::F4;
            case KEY_F5:
                return Key::F5;
            case KEY_F6:
                return Key::F6;
            case KEY_F7:
                return Key::F7;
            case KEY_F8:
                return Key::F8;
            case KEY_F9:
                return Key::F9;
            case KEY_F10:
                return Key::F10;
            case KEY_F11:
                return Key::F11;
            case KEY_F12:
                return Key::F12;
            case KEY_SCROLL_LOCK:
                return Key::ScrollLock;
            case KEY_INSERT:
                return Key::Insert;
            case KEY_HOME:
                return Key::Home;
            case KEY_PAGE_UP:
                return Key::PageUp;
            case KEY_DELETE:
                return Key::Delete;
            case KEY_END:
                return Key::End;
            case KEY_PAGE_DOWN:
                return Key::PageDown;
            case KEY_CURSOR_RIGHT:
                return Key::RightArrow;
            case KEY_CURSOR_LEFT:
                return Key::LeftArrow;
            case KEY_CURSOR_DOWN:
                return Key::DownArrow;
            case KEY_CURSOR_UP:
                return Key::UpArrow;
            case KEY_NUMLOCK:
                return Key::NumLock;
            case KEY_NUMPAD_FORWARD_SLASH:
                return Key::Numpad_Slash;
            case KEY_NUMPAD_STAR:
                return Key::Numpad_Star;
            case KEY_NUMPAD_MINUS:
                return Key::Numpad_Minus;
            case KEY_NUMPAD_PLUS:
                return Key::Numpad_Plus;
            case KEY_NUMPAD_ENTER:
                return Key::Numpad_Enter;
            case KEY_NUMPAD_1:
                return Key::Numpad_1;
            case KEY_NUMPAD_2:
                return Key::Numpad_2;
            case KEY_NUMPAD_3:
                return Key::Numpad_3;
            case KEY_NUMPAD_4:
                return Key::Numpad_4;
            case KEY_NUMPAD_5:
                return Key::Numpad_5;
            case KEY_NUMPAD_6:
                return Key::Numpad_6;
            case KEY_NUMPAD_7:
                return Key::Numpad_7;
            case KEY_NUMPAD_8:
                return Key::Numpad_8;
            case KEY_NUMPAD_9:
                return Key::Numpad_9;
            case KEY_NUMPAD_0:
                return Key::Numpad_0;
            case KEY_NUMPAD_DOT:
                return Key::Numpad_Period;
            case KEY_APPS:
                return Key::Application;
            case KEY_ACPI_POWER:
                return Key::Power;
            case KEY_LEFT_CONTROL:
                return Key::LeftControl;
            case KEY_LEFT_SHIFT:
                return Key::LeftShift;
            case KEY_LEFT_ALT:
                return Key::LeftAlt;
            case KEY_LEFT_GUI:
                return Key::LeftMeta;
            case KEY_RIGHT_CONTROL:
                return Key::RightControl;
            case KEY_RIGHT_SHIFT:
                return Key::RightShift;
            case KEY_RIGHT_ALT:
                return Key::RightAlt;
            case KEY_RIGHT_GUI:
                return Key::RightMeta;
            default:
                return Key::None;
        }
    }();
    return notify_key_event(out_key, out_modifiers, out_generates_text, out_is_down);
}

UniquePtr<KeyEvent> InputTracker::notify_key_event(App::Key key, int modifiers, bool generates_text, bool is_down) {
    if (!is_down) {
        return make_unique<App::KeyUpEvent>(key, modifiers, generates_text, false);
    }

    bool is_multi = false;
    if (m_last_was_multi_keypress_signal) {
        m_last_was_multi_keypress_signal = false;

        timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        auto time_delta_ms = (now.tv_sec - m_multi_keypress_signal_timestamp.tv_sec) * 1000 +
                             (now.tv_nsec - m_multi_keypress_signal_timestamp.tv_nsec) / 1000000;

        is_multi = time_delta_ms <= 2000;
    } else {
        m_last_was_multi_keypress_signal = key == App::Key::K && modifiers == App::KeyModifier::Control;
        if (m_last_was_multi_keypress_signal) {
            clock_gettime(CLOCK_REALTIME, &m_multi_keypress_signal_timestamp);
        }
    }
    return make_unique<App::KeyDownEvent>(key, modifiers, generates_text, is_multi);
}

Vector<UniquePtr<MouseEvent>> InputTracker::notify_os_mouse_event(int scale_mode, int dx, int dy, int dz, int buttons, int screen_width,
                                                                  int screen_height) {
    auto out_x = [&] {
        if (scale_mode == SCALE_ABSOLUTE) {
            return dx * screen_width / 0xFFFF;
        }
        return prev_x() + dx;
    }();
    auto out_y = [&] {
        if (scale_mode == SCALE_ABSOLUTE) {
            return dy * screen_height / 0xFFFF;
        }
        return prev_y() - dy;
    }();

    int out_buttons = 0;
    if (buttons & MOUSE_BUTTON_LEFT) {
        out_buttons |= MouseButton::Left;
    }
    if (buttons & MOUSE_BUTTON_MIDDLE) {
        out_buttons |= MouseButton::Middle;
    }
    if (buttons & MOUSE_BUTTON_RIGHT) {
        out_buttons |= MouseButton::Right;
    }

    return notify_mouse_event(out_buttons, out_x, out_y, dz, 0);
}

Vector<UniquePtr<MouseEvent>> InputTracker::notify_mouse_event(int buttons, int x, int y, int z, int modifiers) {
    Vector<UniquePtr<MouseEvent>> events;
    if (z != 0) {
        events.add(make_unique<MouseScrollEvent>(buttons, x, y, z, MouseButton::None, 0, modifiers));
    }

    if (m_prev.x() != x || m_prev.y() != y) {
        events.add(make_unique<MouseMoveEvent>(m_prev.buttons_down(), x, y, 0, MouseButton::None, 0, modifiers));
    }

    auto buttons_to_pass = m_prev.buttons_down();
    auto handle_button = [&](int button) {
        if (!(buttons & button) && !!(m_prev.buttons_down() & button)) {
            buttons_to_pass &= ~button;
            events.add(make_unique<MouseUpEvent>(buttons_to_pass, x, y, 0, button, 0, modifiers));
        }

        if (!!(buttons & button) && !(m_prev.buttons_down() & button)) {
            auto count = m_prev.set(x, y, button);
            buttons_to_pass |= button;
            events.add(make_unique<MouseDownEvent>(buttons_to_pass, x, y, 0, button, count, modifiers));
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

int InputTracker::MousePress::set(int x, int y, int button) {
    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    auto time_delta_ms = (now.tv_sec - m_timestamp.tv_sec) * 1000 + (now.tv_nsec - m_timestamp.tv_nsec) / 1000000;
    bool is_repetition = m_button == button && m_x == x && m_y == y && time_delta_ms <= 500;

    m_button = button;
    m_timestamp = now;

    if (!is_repetition) {
        m_count = 0;
    }
    return ++m_count;
}
}
