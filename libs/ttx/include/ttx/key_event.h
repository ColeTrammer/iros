#pragma once

#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/reflect/enumerator.h"
#include "di/reflect/field.h"
#include "di/reflect/prelude.h"
#include "ttx/key.h"
#include "ttx/modifiers.h"

namespace ttx {
enum class KeyEventType {
    Press = 1,
    Repeat = 2,
    Release = 3,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<KeyEventType>) {
    using enum KeyEventType;
    return di::make_enumerators<"KeyEventType">(di::enumerator<"Press", Press>, di::enumerator<"Repeat", Repeat>,
                                                di::enumerator<"Release", Release>);
}

class KeyEvent {
public:
    constexpr static auto key_down(Key key, di::String text = {}, Modifiers modifiers = Modifiers::None,
                                   c32 shifted_key = 0, c32 base_layout_key = 0) -> KeyEvent {
        return { KeyEventType::Press, key, di::move(text), modifiers, shifted_key, base_layout_key };
    }

    constexpr KeyEvent(KeyEventType type, Key key, di::String text = {}, Modifiers modifiers = Modifiers::None,
                       c32 shifted_key = 0, c32 base_layout_key = 0)
        : m_type(type)
        , m_modifiers(modifiers)
        , m_key(key)
        , m_shifted_key(shifted_key)
        , m_base_layout_key(base_layout_key)
        , m_text(di::move(text)) {}

    constexpr auto type() const -> KeyEventType { return m_type; }
    constexpr auto modifiers() const -> Modifiers { return m_modifiers; }
    constexpr auto key() const -> Key { return m_key; }
    constexpr auto shifted_key() const -> c32 { return m_shifted_key; }
    constexpr auto base_layout_key() const -> c32 { return m_base_layout_key; }
    constexpr auto text() const -> di::StringView const { return m_text; }

    auto operator==(KeyEvent const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<KeyEvent>) {
        return di::make_fields<"KeyEvent">(
            di::field<"type", &KeyEvent::m_type>, di::field<"modifers", &KeyEvent::m_modifiers>,
            di::field<"key", &KeyEvent::m_key>, di::field<"shifted_key", &KeyEvent::m_shifted_key>,
            di::field<"base_layout_key", &KeyEvent::m_base_layout_key>, di::field<"text", &KeyEvent::m_text>);
    }

    KeyEventType m_type { KeyEventType::Press };
    Modifiers m_modifiers { Modifiers::None };
    Key m_key { Key::None };
    c32 m_shifted_key { 0 };
    c32 m_base_layout_key { 0 };
    di::String m_text;
};
}
