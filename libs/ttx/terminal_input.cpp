#include "ttx/terminal_input.h"

#include "di/container/algorithm/for_each.h"
#include "ttx/key_event.h"
#include "ttx/key_event_io.h"
#include "ttx/modifiers.h"

namespace ttx {
auto TerminalInputParser::parse(di::StringView input) -> di::Vector<Event> {
    auto event = m_parser.parse_input_escape_sequences(input);
    di::for_each(event, [&](auto const& ev) {
        di::visit(
            [&](auto const& e) {
                this->handle(e);
            },
            ev);
    });
    return di::move(m_events);
}

void TerminalInputParser::handle(PrintableCharacter const& printable_character) {
    m_events.emplace_back(key_event_from_legacy_code_point(printable_character.code_point));
}

void TerminalInputParser::handle(DCS const&) {}

void TerminalInputParser::handle(CSI const& csi) {
    if (auto key_event = key_event_from_csi(csi)) {
        m_events.emplace_back(di::move(key_event).value());
    }
}

void TerminalInputParser::handle(Escape const&) {}

void TerminalInputParser::handle(ControlCharacter const& control_character) {
    auto modifiers = control_character.was_in_escape ? Modifiers::Alt : Modifiers::None;
    m_events.emplace_back(key_event_from_legacy_code_point(control_character.code_point, modifiers));
}
}
