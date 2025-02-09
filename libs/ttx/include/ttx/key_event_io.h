#pragma once

#include "di/reflect/prelude.h"
#include "di/util/bitwise_enum.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/key_event.h"
#include "ttx/params.h"

namespace ttx {
// Application cursor keys mode. This controls whether to report the certain
// keys using a CSI or SS3 when no modifiers are needed. For instance,
// when enabled, the home key is reported as: SS3 H. While when not enabled,
// the home key is reported as CSI H.
enum class ApplicationCursorKeysMode {
    Disabled,
    Enabled,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ApplicationCursorKeysMode>) {
    using enum ApplicationCursorKeysMode;
    return di::make_enumerators<"ApplicationCursorKeysMode">(di::enumerator<"Disabled", Disabled>,
                                                             di::enumerator<"Enabled", Enabled>);
}

// Key reporting flags from Kitty keyboard protocol:
// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
enum class KeyReportingFlags {
    None = 0,
    Disambiguate = 1 << 0,
    ReportEventTypes = 1 << 1,
    ReportAlternateKeys = 1 << 2,
    ReportAllKeysAsEscapeCodes = 1 << 3,
    ReportAssociatedText = 1 << 4,
    All = Disambiguate | ReportEventTypes | ReportAlternateKeys | ReportAllKeysAsEscapeCodes | ReportAssociatedText,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(KeyReportingFlags)

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<KeyReportingFlags>) {
    using enum KeyReportingFlags;
    return di::make_enumerators<"KeyReportingFlags">(
        di::enumerator<"None", None>, di::enumerator<"Disambiguate", Disambiguate>,
        di::enumerator<"ReportEventTypes", ReportEventTypes>,
        di::enumerator<"ReportAlternateKeys", ReportAlternateKeys>,
        di::enumerator<"ReportAllKeysAsEscapeCodes", ReportAllKeysAsEscapeCodes>,
        di::enumerator<"ReportAssociatedText", ReportAssociatedText>);
}

auto serialize_key_event(KeyEvent const& event, ApplicationCursorKeysMode cursor_key_mode, KeyReportingFlags flags)
    -> di::Optional<di::String>;

auto key_event_from_legacy_code_point(c32 code_point, Modifiers base_modifiers = Modifiers::None) -> KeyEvent;
auto key_event_from_csi(CSI const& csi) -> di::Optional<KeyEvent>;
}
