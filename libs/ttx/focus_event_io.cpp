#include "ttx/focus_event_io.h"

#include "ttx/escape_sequence_parser.h"
#include "ttx/focus_event.h"

namespace ttx {
auto serialize_focus_event(FocusEvent const& focus_event, FocusEventMode mode) -> di::Optional<di::String> {
    if (mode == FocusEventMode::Disabled) {
        return {};
    }
    if (focus_event.is_focus_in()) {
        return "\033[I"_s;
    } else {
        return "\033[O"_s;
    }
}

auto focus_event_from_csi(CSI const& csi) -> di::Optional<FocusEvent> {
    if (csi == CSI(""_s, {}, U'I')) {
        return FocusEvent::focus_in();
    }
    if (csi == CSI(""_s, {}, U'O')) {
        return FocusEvent::focus_out();
    }
    return {};
}
}
