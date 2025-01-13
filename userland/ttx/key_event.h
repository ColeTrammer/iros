#pragma once

#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/reflect/enumerator.h"
#include "di/reflect/field.h"
#include "di/reflect/prelude.h"
#include "key.h"

namespace ttx {
enum class KeyEventType {
    Press = 1,
    Repeat = 2,
    Release = 3,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<KeyEventType>) {
    using enum KeyEventType;
    return di::make_enumerators(di::enumerator<"Press", Press>, di::enumerator<"Repeat", Repeat>,
                                di::enumerator<"Release", Release>);
}

class KeyEvent {
public:
    constexpr static auto key_down(Key key, di::String text = {}, Modifiers modifiers = Modifiers::None) -> KeyEvent {
        return { KeyEventType::Press, key, di::move(text), modifiers };
    }

    constexpr KeyEvent(KeyEventType type, Key key, di::String text, Modifiers modifiers)
        : m_type(type), m_modifiers(modifiers), m_key(key), m_text(di::move(text)) {}

    constexpr auto type() const -> KeyEventType { return m_type; }
    constexpr auto modifiers() const -> Modifiers { return m_modifiers; }
    constexpr auto key() const -> Key { return m_key; }
    constexpr auto text() -> di::StringView const { return m_text; }

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<KeyEvent>) {
        return di::make_fields(di::field<"type", &KeyEvent::m_type>, di::field<"modifers", &KeyEvent::m_modifiers>,
                               di::field<"key", &KeyEvent::m_key>, di::field<"text", &KeyEvent::m_text>);
    }

    KeyEventType m_type { KeyEventType::Press };
    Modifiers m_modifiers { Modifiers::None };
    Key m_key { Key::None };
    di::String m_text;
};
}
