#include "ttx/terminal_input.h"

#include "di/reflect/valid_enum_value.h"
#include "ttx/key.h"
#include "ttx/key_event.h"
#include "ttx/key_event_io.h"
#include "ttx/params.h"

namespace ttx {
auto TerminalInputParser::parse(di::StringView input) -> di::Vector<Event> {
    for (auto code_point : input) {
        handle_code_point(code_point);
    }

    // Special case: if we a lone "escape" byte followed by no text,
    // we assume the user hit the escape key, and not that the terminal
    // partially transmitted an escape sequence.
    //
    // When using kitty input protocol, this ambiguity is avoided.
    if (m_state == State::Escape) {
        emit(key_event_from_legacy_code_point('\x1b'));
        m_state = State::Base;
    }
    return di::move(m_pending_events);
}

void TerminalInputParser::handle_code_point(c32 input) {
    switch (m_state) {
        case State::Base:
            return handle_base(input);
        case State::Escape:
            return handle_escape(input);
        case State::CSI:
            return handle_csi(input);
        case State::SS3:
            return handle_ss3(input);
    }
}

void TerminalInputParser::handle_base(c32 input) {
    // Reset accumulator
    m_accumulator.clear();

    // Handle escape key.
    if (input == U'\033') {
        m_state = State::Escape;
        return;
    }

    emit(key_event_from_legacy_code_point(input));
}

void TerminalInputParser::handle_escape(c32 input) {
    switch (input) {
        case '[':
            m_state = State::CSI;
            break;
        case 'O':
            m_state = State::SS3;
            break;
        default:
            // Escape followed by any other character represents the key was pressed with alt held down.
            emit(key_event_from_legacy_code_point(input, Modifiers::Alt));
            m_state = State::Base;
            break;
    }
}

void TerminalInputParser::handle_csi(c32 input) {
    // Check if input does not end a key press.
    auto is_digit = input >= U'0' && input <= U'9';
    if (input == U';' || input == U':' || is_digit) {
        m_accumulator.push_back(input);
        return;
    }

    // Parse the params.
    auto params = Params::from_string(m_accumulator);
    key_event_from_csi(params, input).transform(di::bind_front(&TerminalInputParser::emit, this));
    m_state = State::Base;
}

void TerminalInputParser::handle_ss3(c32 input) {
    key_event_from_csi({}, input).transform(di::bind_front(&TerminalInputParser::emit, this));
    m_state = State::Base;
}
}
