#pragma once

#include <eventloop/event.h>
#include <eventloop/widget_events.h>
#include <liim/function.h>
#include <liim/vector.h>

namespace App {
class KeyShortcut {
public:
    enum class IsMulti { Yes, No };

    KeyShortcut(Key key, int modifiers, IsMulti is_multi = IsMulti::No)
        : m_key(key), m_modifiers(modifiers), m_is_multi(is_multi == IsMulti::Yes) {}

    Key key() const { return m_key; }
    int modifiers() const { return m_modifiers; }
    bool is_multi() const { return m_is_multi; }

private:
    Key m_key { Key::None };
    int m_modifiers { 0 };
    bool m_is_multi { false };
};

class KeyBindings {
public:
    void add(const KeyShortcut& shortcut, Function<void()> action, Function<bool()> filter = nullptr);

    bool handle_key_event(const KeyEvent& event);

private:
    struct KeyBinding {
        KeyShortcut shortcut;
        Function<void()> action;
        Function<bool()> filter;
    };

    Vector<KeyBinding> m_key_bindings;
};
}
