#pragma once

#include <edit/forward.h>
#include <eventloop/key_bindings.h>
#include <liim/string_view.h>

namespace Edit {
class KeyboardAction {
public:
    enum class Type {
        Document,
        Display,
    };

    KeyboardAction(Type type, StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action);

    App::KeyShortcut shortcut() const { return m_shortcut; }
    StringView name() const { return m_name; }
    Type type() const { return m_type; }

    void do_action(Display& display) { m_action(display); }

private:
    Type m_type;
    StringView m_name;
    App::KeyShortcut m_shortcut;
    Function<void(Display&)> m_action;
};

void register_document_keyboard_action(StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action);
void register_display_keyboard_action(StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action);
App::KeyBindings get_key_bindings(Display& display);
}
