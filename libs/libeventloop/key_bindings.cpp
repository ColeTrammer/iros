#include <eventloop/key_bindings.h>

namespace App {
void KeyBindings::add(const KeyShortcut& shortcut, Function<void()> action, Function<bool()> filter) {
    m_key_bindings.add({ shortcut, move(action), move(filter) });
}

bool KeyBindings::handle_key_event(const KeyEvent& event) {
    if (!event.key_down()) {
        return false;
    }

    for (auto& binding : m_key_bindings) {
        if (event.key() != binding.shortcut.key() || event.modifiers() != binding.shortcut.modifiers()) {
            continue;
        }

        if (binding.filter && !binding.filter()) {
            continue;
        }

        binding.action();
        return true;
    }
    return false;
}
}
