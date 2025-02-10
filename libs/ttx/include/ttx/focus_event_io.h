#pragma once

#include "di/reflect/prelude.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/focus_event.h"

// Focus event reference: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-FocusIn_FocusOut
namespace ttx {
// Focus tracking. When enabled, we send focus events to the application.
enum class FocusEventMode {
    Disabled,
    Enabled,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<FocusEventMode>) {
    using enum FocusEventMode;
    return di::make_enumerators<"FocusEventMode">(di::enumerator<"Disabled", Disabled>,
                                                  di::enumerator<"Enabled", Enabled>);
}

auto serialize_focus_event(FocusEvent const& focus_event, FocusEventMode mode) -> di::Optional<di::String>;
auto focus_event_from_csi(CSI const& csi) -> di::Optional<FocusEvent>;
}
