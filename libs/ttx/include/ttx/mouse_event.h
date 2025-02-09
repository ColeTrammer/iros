#pragma once

#include "di/reflect/enumerator.h"
#include "di/reflect/field.h"
#include "di/reflect/prelude.h"
#include "ttx/modifiers.h"
#include "ttx/mouse.h"

namespace ttx {
enum class MouseEventType {
    Press = 1,
    Move = 2,
    Release = 3,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseEventType>) {
    using enum MouseEventType;
    return di::make_enumerators<"KeyEventType">(di::enumerator<"Press", Press>, di::enumerator<"Move", Move>,
                                                di::enumerator<"Release", Release>);
}

class MouseEvent {
public:
    constexpr static auto press(MouseButton button, MousePosition const& position = {},
                                Modifiers modifiers = Modifiers::None) -> MouseEvent {
        return { MouseEventType::Press, button, position, modifiers };
    }

    constexpr MouseEvent(MouseEventType type, MouseButton button, MousePosition const& position = {},
                         Modifiers modifiers = Modifiers::None)
        : m_type(type), m_button(button), m_position(position), m_modifiers(modifiers) {}

    constexpr auto type() const -> MouseEventType { return m_type; }
    constexpr auto button() const -> MouseButton { return m_button; }
    constexpr auto position() const -> MousePosition const& { return m_position; }
    constexpr auto modifiers() const -> Modifiers { return m_modifiers; }

    constexpr auto is_vertical_scroll() const -> bool { return !!(button() & MouseButton::VerticalScrollButtons); }

    auto operator==(MouseEvent const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseEvent>) {
        return di::make_fields<"MouseEvent">(
            di::field<"type", &MouseEvent::m_type>, di::field<"button", &MouseEvent::m_button>,
            di::field<"position", &MouseEvent::m_position>, di::field<"modifers", &MouseEvent::m_modifiers>);
    }

    MouseEventType m_type { MouseEventType::Press };
    MouseButton m_button { MouseButton::None };
    MousePosition m_position;
    Modifiers m_modifiers { Modifiers::None };
};
}
