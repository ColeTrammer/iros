#include <edit/actions.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/keyboard_action.h>

namespace Edit {
static Vector<KeyboardAction> s_actions;
static bool s_initialized { false };

KeyboardAction::KeyboardAction(Type type, StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action)
    : m_type(type), m_name(name), m_shortcut(default_shortcut), m_action(move(action)) {}

void register_document_keyboard_action(StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action) {
    s_actions.add(KeyboardAction { KeyboardAction::Type::Document, name, default_shortcut, move(action) });
}

void register_display_keyboard_action(StringView name, App::KeyShortcut default_shortcut, Function<void(Display&)> action) {
    s_actions.add(KeyboardAction { KeyboardAction::Type::Display, name, default_shortcut, move(action) });
}

App::KeyBindings get_key_bindings(Display& display) {
    if (!s_initialized) {
        init_actions();
        s_initialized = true;
    }

    auto key_bindings = App::KeyBindings {};
    for (auto& action : s_actions) {
        key_bindings.add(
            action.shortcut(),
            [&action, &display] {
                display.start_input(action.name() != "Cursor Undo");
                action.do_action(display);
                display.finish_input(action.type() == KeyboardAction::Type::Document);
            },
            [&action, &display] {
                auto* document = display.document();
                if (!document) {
                    return false;
                }
                if (action.type() == KeyboardAction::Type::Display && document->input_text_mode()) {
                    return false;
                }
                return true;
            });
    }
    return key_bindings;
}
}
