#pragma once

#include "di/container/string/string_view.h"
#include "di/reflect/prelude.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/paste_event.h"

// Bracketed paste reference: https://invisible-island.net/xterm/xterm-paste64.html
namespace ttx {
// Bracketed paste mode. When enabled, we send focus events to the application.
enum class BracketedPasteMode {
    Disabled,
    Enabled,
};

constexpr auto bracketed_paste_end = "\033[201~"_sv;

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<BracketedPasteMode>) {
    using enum BracketedPasteMode;
    return di::make_enumerators<"BracketedPasteMode">(di::enumerator<"Disabled", Disabled>,
                                                      di::enumerator<"Enabled", Enabled>);
}

auto serialize_paste_event(PasteEvent const& event, BracketedPasteMode mode) -> di::String;

auto is_bracketed_paste_begin(CSI const& csi) -> bool;
}
