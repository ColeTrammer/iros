#pragma once

#include "di/container/string/string.h"
#include "di/reflect/field.h"
#include "di/reflect/prelude.h"

namespace ttx {
class PasteEvent {
public:
    constexpr explicit PasteEvent(di::String text) : m_text(di::move(text)) {}

    auto text() const -> di::StringView { return m_text; }

    auto operator==(PasteEvent const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PasteEvent>) {
        return di::make_fields<"PasteEvent">(di::field<"text", &PasteEvent::m_text>);
    }

    di::String m_text;
};
}
