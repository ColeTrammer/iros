#pragma once

#include <eventloop/event.h>
#include <eventloop/widget_events.h>
#include <liim/function.h>
#include <liim/vector.h>

namespace App {
class KeyShortcut {
public:
    KeyShortcut(Key key, int modifiers) : m_key(key), m_modifiers(modifiers) {}

    Key key() const { return m_key; }
    int modifiers() const { return m_modifiers; }

private:
    Key m_key { Key::None };
    int m_modifiers { 0 };
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
