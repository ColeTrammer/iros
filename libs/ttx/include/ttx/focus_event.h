#pragma once

#include "di/reflect/field.h"
#include "di/reflect/prelude.h"

namespace ttx {
class FocusEvent {
public:
    constexpr static auto focus_in() -> FocusEvent { return { true }; }
    constexpr static auto focus_out() -> FocusEvent { return { false }; }

    constexpr FocusEvent(bool gained_focus) : m_gained_focus(gained_focus) {}

    constexpr auto is_focus_in() const { return m_gained_focus; }
    constexpr auto is_focus_out() const { return !m_gained_focus; }

    auto operator==(FocusEvent const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<FocusEvent>) {
        return di::make_fields<"FocusEvent">(di::field<"gained_focus", &FocusEvent::m_gained_focus>);
    }

    bool m_gained_focus { false };
};
}
