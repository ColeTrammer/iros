#include "ttx/paste_event_io.h"

#include "di/format/prelude.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/paste_event.h"

namespace ttx {
auto serialize_paste_event(PasteEvent const& event, BracketedPasteMode mode) -> di::String {
    // TODO: consider sanitizing paste events to prevent issues, such as embedding
    // escape sequences (specifically the bracketed paste terminator). For now,
    // we're relying on the outer terminal to do this.
    if (mode == BracketedPasteMode::Disabled) {
        return event.text().to_owned();
    }
    return *di::present("\033[200~{}\033[201~"_sv, event.text());
}

auto is_bracketed_paste_begin(CSI const& csi) -> bool {
    return csi == CSI(""_s, { { 200 } }, U'~');
}
}
