#pragma once

#include "di/reflect/prelude.h"
#include "dius/tty.h"
#include "ttx/key_event_io.h"
#include "ttx/mouse.h"
#include "ttx/mouse_event.h"
#include "ttx/params.h"

// Mouse reference: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
namespace ttx {
// Alternate scroll mode - when enabled, scroll events are reported using cursor up/down
// escape sequences. This only applies when the alternate screen buffer is active.
enum class AlternateScrollMode {
    Disabled,
    Enabled,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<AlternateScrollMode>) {
    using enum AlternateScrollMode;
    return di::make_enumerators<"AlternateScrollMode">(di::enumerator<"Disabled", Disabled>,
                                                       di::enumerator<"Enabled", Enabled>);
}

// Mouse protocol - determines which mouse events are forwarded to the application.
//   The enum values are determined by the terminal mode which enables them.
enum class MouseProtocol {
    None = 0,
    X10 = 9,
    VT200 = 1000,
    BtnEvent = 1002,
    AnyEvent = 1003,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseProtocol>) {
    using enum MouseProtocol;
    return di::make_enumerators<"MouseProtocol">(di::enumerator<"None", None>, di::enumerator<"X10", X10>,
                                                 di::enumerator<"VT200", VT200>, di::enumerator<"BtnEvent", BtnEvent>,
                                                 di::enumerator<"AnyEvent", AnyEvent>);
}

// Mouse encoding - determines the bytes sent to the application when an event is forwarded.
//   The enum values are determined by the terminal mode which enables them.
enum class MouseEncoding {
    X10 = 9,
    UTF8 = 1005,
    SGR = 1006,
    URXVT = 1015,
    SGRPixels = 1016,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseEncoding>) {
    using enum MouseEncoding;
    return di::make_enumerators<"MouseEncoding">(di::enumerator<"X10", X10>, di::enumerator<"UTF8", UTF8>,
                                                 di::enumerator<"SGR", SGR>, di::enumerator<"URXVT", URXVT>,
                                                 di::enumerator<"SGRPixels", SGRPixels>);
}

// Mouse scroll protocol - controls how the vertical scroll buttons are sent in cases
//   where mouse reporting otherwise wouldn't happen.
struct MouseScrollProtocol {
    AlternateScrollMode alternate_scroll_mode { AlternateScrollMode::Disabled };
    ApplicationCursorKeysMode application_cursor_keys_mode { ApplicationCursorKeysMode::Disabled };
    bool in_alternate_screen_buffer { false };
};

auto serialize_mouse_event(MouseEvent const& event, MouseProtocol protocol, MouseEncoding encoding,
                           di::Optional<MousePosition> prev_event_position, MouseScrollProtocol const& scroll_protocol,
                           dius::tty::WindowSize const& window_size) -> di::Optional<di::TransparentString>;
auto parse_sgr_mouse_event(Params const& params, c32 final_code_point,
                           di::Optional<dius::tty::WindowSize> window_size_if_using_pixels = {})
    -> di::Optional<MouseEvent>;
}
