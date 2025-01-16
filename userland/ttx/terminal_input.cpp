#include "terminal_input.h"

#include "di/reflect/valid_enum_value.h"
#include "di/vocab/array/to_array.h"
#include "key.h"
#include "key_event.h"
#include "ttx/params.h"

namespace ttx {
struct CodePointMapping {
    c32 code_point { 0 };
    Key key { Key::None };
    Modifiers modifiers = Modifiers::None;
};

constexpr auto legacy_code_point_mappings = di::to_array<CodePointMapping>({
    { 0x00, Key::Space, Modifiers::Control },
    { 0x01, Key::A, Modifiers::Control },
    { 0x02, Key::B, Modifiers::Control },
    { 0x03, Key::C, Modifiers::Control },
    { 0x04, Key::D, Modifiers::Control },
    { 0x05, Key::E, Modifiers::Control },
    { 0x06, Key::F, Modifiers::Control },
    { 0x07, Key::G, Modifiers::Control },
    { 0x08, Key::Backspace, Modifiers::Control },
    { 0x09, Key::Tab, Modifiers::None },
    { 0x0a, Key::J, Modifiers::Control },
    { 0x0b, Key::K, Modifiers::Control },
    { 0x0c, Key::L, Modifiers::Control },
    { 0x0d, Key::Enter, Modifiers::Control },
    { 0x0e, Key::N, Modifiers::Control },
    { 0x0f, Key::O, Modifiers::Control },
    { 0x10, Key::P, Modifiers::Control },
    { 0x11, Key::Q, Modifiers::Control },
    { 0x12, Key::R, Modifiers::Control },
    { 0x13, Key::S, Modifiers::Control },
    { 0x14, Key::T, Modifiers::Control },
    { 0x15, Key::U, Modifiers::Control },
    { 0x16, Key::V, Modifiers::Control },
    { 0x17, Key::W, Modifiers::Control },
    { 0x18, Key::X, Modifiers::Control },
    { 0x19, Key::Y, Modifiers::Control },
    { 0x1a, Key::Z, Modifiers::Control },
    { 0x1b, Key::Escape, Modifiers::None },
    { 0x1c, Key::BackSlash, Modifiers::Control },
    { 0x1d, Key::RightBracket, Modifiers::Control },
    { 0x1e, Key::_6, Modifiers::Control },
    { 0x1f, Key::_7, Modifiers::Control },

    { ' ', Key::Space, Modifiers::None },
    { '!', Key::_1, Modifiers::Shift },
    { '"', Key::Quote, Modifiers::Shift },
    { '#', Key::_3, Modifiers::Shift },
    { '$', Key::_4, Modifiers::Shift },
    { '%', Key::_5, Modifiers::Shift },
    { '&', Key::_7, Modifiers::Shift },
    { '\'', Key::Quote, Modifiers::None },
    { '(', Key::_9, Modifiers::Shift },
    { ')', Key::_0, Modifiers::Shift },
    { '*', Key::_8, Modifiers::Shift },
    { '+', Key::Equal, Modifiers::Shift },
    { ',', Key::Comma, Modifiers::None },
    { '-', Key::Minus, Modifiers::None },
    { '.', Key::D, Modifiers::None },
    { '/', Key::Slash, Modifiers::None },
    { '0', Key::_0, Modifiers::None },
    { '1', Key::_1, Modifiers::None },
    { '2', Key::_2, Modifiers::None },
    { '3', Key::_3, Modifiers::None },
    { '4', Key::_4, Modifiers::None },
    { '5', Key::_5, Modifiers::None },
    { '6', Key::_6, Modifiers::None },
    { '7', Key::_7, Modifiers::None },
    { '8', Key::_8, Modifiers::None },
    { '9', Key::_9, Modifiers::None },
    { ':', Key::SemiColon, Modifiers::Shift },
    { ';', Key::SemiColon, Modifiers::None },
    { '<', Key::Comma, Modifiers::Shift },
    { '=', Key::Equal, Modifiers::None },
    { '>', Key::Period, Modifiers::Shift },
    { '?', Key::Slash, Modifiers::Shift },
    { '@', Key::_2, Modifiers::Shift },
    { 'A', Key::A, Modifiers::Shift },
    { 'B', Key::B, Modifiers::Shift },
    { 'C', Key::C, Modifiers::Shift },
    { 'D', Key::D, Modifiers::Shift },
    { 'E', Key::E, Modifiers::Shift },
    { 'F', Key::F, Modifiers::Shift },
    { 'G', Key::G, Modifiers::Shift },
    { 'H', Key::H, Modifiers::Shift },
    { 'I', Key::I, Modifiers::Shift },
    { 'J', Key::J, Modifiers::Shift },
    { 'K', Key::K, Modifiers::Shift },
    { 'L', Key::L, Modifiers::Shift },
    { 'M', Key::M, Modifiers::Shift },
    { 'N', Key::N, Modifiers::Shift },
    { 'O', Key::O, Modifiers::Shift },
    { 'P', Key::P, Modifiers::Shift },
    { 'Q', Key::Q, Modifiers::Shift },
    { 'R', Key::R, Modifiers::Shift },
    { 'S', Key::S, Modifiers::Shift },
    { 'T', Key::T, Modifiers::Shift },
    { 'U', Key::U, Modifiers::Shift },
    { 'V', Key::V, Modifiers::Shift },
    { 'W', Key::W, Modifiers::Shift },
    { 'X', Key::X, Modifiers::Shift },
    { 'Y', Key::Y, Modifiers::Shift },
    { 'Z', Key::Z, Modifiers::Shift },
    { '[', Key::LeftBracket, Modifiers::None },
    { '\\', Key::BackSlash, Modifiers::None },
    { ']', Key::RightBracket, Modifiers::None },
    { '^', Key::_6, Modifiers::Shift },
    { '_', Key::Minus, Modifiers::Shift },
    { '`', Key::Backtick, Modifiers::None },
    { 'a', Key::A, Modifiers::None },
    { 'b', Key::B, Modifiers::None },
    { 'c', Key::C, Modifiers::None },
    { 'd', Key::D, Modifiers::None },
    { 'e', Key::E, Modifiers::None },
    { 'f', Key::F, Modifiers::None },
    { 'g', Key::G, Modifiers::None },
    { 'h', Key::H, Modifiers::None },
    { 'i', Key::I, Modifiers::None },
    { 'j', Key::J, Modifiers::None },
    { 'k', Key::K, Modifiers::None },
    { 'l', Key::L, Modifiers::None },
    { 'm', Key::M, Modifiers::None },
    { 'n', Key::N, Modifiers::None },
    { 'o', Key::O, Modifiers::None },
    { 'p', Key::P, Modifiers::None },
    { 'q', Key::Q, Modifiers::None },
    { 'r', Key::R, Modifiers::None },
    { 's', Key::S, Modifiers::None },
    { 't', Key::T, Modifiers::None },
    { 'u', Key::U, Modifiers::None },
    { 'v', Key::V, Modifiers::None },
    { 'w', Key::W, Modifiers::None },
    { 'x', Key::X, Modifiers::None },
    { 'y', Key::Y, Modifiers::None },
    { 'z', Key::Z, Modifiers::None },
    { '{', Key::LeftBracket, Modifiers::Shift },
    { '|', Key::BackSlash, Modifiers::Shift },
    { '}', Key::RightBracket, Modifiers::Shift },
    { '~', Key::Backtick, Modifiers::Shift },

    { 0x7f, Key::Backspace, Modifiers::None },
});

constexpr auto ss3_mappings = di::to_array<CodePointMapping>({
    { 'A', Key::Up, Modifiers::None },
    { 'B', Key::Down, Modifiers::None },
    { 'C', Key::Right, Modifiers::None },
    { 'D', Key::Left, Modifiers::None },
    { 'E', Key::KeyPadBegin, Modifiers::None },
    { 'H', Key::Home, Modifiers::None },
    { 'F', Key::End, Modifiers::None },
    { 'P', Key::F1, Modifiers::None },
    { 'Q', Key::F2, Modifiers::None },
    { 'R', Key::F3, Modifiers::None },
    { 'S', Key::F4, Modifiers::None },
});

constexpr auto legacy_functional_key_mappings = di::to_array<CodePointMapping>({
    { 2, Key::Insert, Modifiers::None },   { 3, Key::Delete, Modifiers::None }, { 5, Key::PageUp, Modifiers::None },
    { 6, Key::PageDown, Modifiers::None }, { 7, Key::Home, Modifiers::None },   { 8, Key::End, Modifiers::None },
    { 11, Key::F1, Modifiers::None },      { 12, Key::F2, Modifiers::None },    { 13, Key::F3, Modifiers::None },
    { 14, Key::F4, Modifiers::None },      { 15, Key::F5, Modifiers::None },    { 17, Key::F6, Modifiers::None },
    { 18, Key::F7, Modifiers::None },      { 19, Key::F8, Modifiers::None },    { 20, Key::F9, Modifiers::None },
    { 21, Key::F10, Modifiers::None },     { 23, Key::F11, Modifiers::None },   { 24, Key::F12, Modifiers::None },
    { 29, Key::Menu, Modifiers::None },
});

constexpr auto code_point_key_mappings = di::to_array<CodePointMapping>({
    { 9, Key::Tab, Modifiers::None },
    { 13, Key::Enter, Modifiers::None },
    { 27, Key::Escape, Modifiers::None },
    { ' ', Key::Space, Modifiers::None },
    { '\'', Key::Quote, Modifiers::None },
    { ',', Key::Comma, Modifiers::None },
    { '-', Key::Minus, Modifiers::None },
    { '.', Key::D, Modifiers::None },
    { '/', Key::Slash, Modifiers::None },
    { '0', Key::_0, Modifiers::None },
    { '1', Key::_1, Modifiers::None },
    { '2', Key::_2, Modifiers::None },
    { '3', Key::_3, Modifiers::None },
    { '4', Key::_4, Modifiers::None },
    { '5', Key::_5, Modifiers::None },
    { '6', Key::_6, Modifiers::None },
    { '7', Key::_7, Modifiers::None },
    { '8', Key::_8, Modifiers::None },
    { '9', Key::_9, Modifiers::None },
    { ';', Key::SemiColon, Modifiers::None },
    { '=', Key::Equal, Modifiers::None },
    { '[', Key::LeftBracket, Modifiers::None },
    { '\\', Key::BackSlash, Modifiers::None },
    { ']', Key::RightBracket, Modifiers::None },
    { '`', Key::Backtick, Modifiers::None },
    { 'a', Key::A, Modifiers::None },
    { 'b', Key::B, Modifiers::None },
    { 'c', Key::C, Modifiers::None },
    { 'd', Key::D, Modifiers::None },
    { 'e', Key::E, Modifiers::None },
    { 'f', Key::F, Modifiers::None },
    { 'g', Key::G, Modifiers::None },
    { 'h', Key::H, Modifiers::None },
    { 'i', Key::I, Modifiers::None },
    { 'j', Key::J, Modifiers::None },
    { 'k', Key::K, Modifiers::None },
    { 'l', Key::L, Modifiers::None },
    { 'm', Key::M, Modifiers::None },
    { 'n', Key::N, Modifiers::None },
    { 'o', Key::O, Modifiers::None },
    { 'p', Key::P, Modifiers::None },
    { 'q', Key::Q, Modifiers::None },
    { 'r', Key::R, Modifiers::None },
    { 's', Key::S, Modifiers::None },
    { 't', Key::T, Modifiers::None },
    { 'u', Key::U, Modifiers::None },
    { 'v', Key::V, Modifiers::None },
    { 'w', Key::W, Modifiers::None },
    { 'x', Key::X, Modifiers::None },
    { 'y', Key::Y, Modifiers::None },
    { 'z', Key::Z, Modifiers::None },
    { 127, Key::Backspace, Modifiers::None },

    { 57358, Key::CapsLock, Modifiers::None },
    { 57359, Key::ScrollLock, Modifiers::None },
    { 57360, Key::NumLock, Modifiers::None },
    { 57361, Key::PrintScreen, Modifiers::None },
    { 57362, Key::Pause, Modifiers::None },
    { 57363, Key::Menu, Modifiers::None },
    { 57376, Key::F13, Modifiers::None },
    { 57377, Key::F14, Modifiers::None },
    { 57378, Key::F15, Modifiers::None },
    { 57379, Key::F16, Modifiers::None },
    { 57380, Key::F17, Modifiers::None },
    { 57381, Key::F18, Modifiers::None },
    { 57382, Key::F19, Modifiers::None },
    { 57383, Key::F20, Modifiers::None },
    { 57384, Key::F21, Modifiers::None },
    { 57385, Key::F22, Modifiers::None },
    { 57386, Key::F23, Modifiers::None },
    { 57387, Key::F24, Modifiers::None },
    { 57388, Key::F25, Modifiers::None },
    { 57389, Key::F26, Modifiers::None },
    { 57390, Key::F27, Modifiers::None },
    { 57391, Key::F28, Modifiers::None },
    { 57392, Key::F29, Modifiers::None },
    { 57393, Key::F30, Modifiers::None },
    { 57394, Key::F31, Modifiers::None },
    { 57395, Key::F32, Modifiers::None },
    { 57396, Key::F33, Modifiers::None },
    { 57397, Key::F34, Modifiers::None },
    { 57398, Key::F13, Modifiers::None },
    { 57399, Key::KeyPad0, Modifiers::None },
    { 57400, Key::KeyPad1, Modifiers::None },
    { 57401, Key::KeyPad2, Modifiers::None },
    { 57402, Key::KeyPad3, Modifiers::None },
    { 57403, Key::KeyPad4, Modifiers::None },
    { 57404, Key::KeyPad5, Modifiers::None },
    { 57405, Key::KeyPad6, Modifiers::None },
    { 57406, Key::KeyPad7, Modifiers::None },
    { 57407, Key::KeyPad8, Modifiers::None },
    { 57408, Key::KeyPad9, Modifiers::None },
    { 57409, Key::KeyPadDecimal, Modifiers::None },
    { 57410, Key::KeyPadDivide, Modifiers::None },
    { 57411, Key::KeyPadMultiply, Modifiers::None },
    { 57412, Key::KeyPadSubtract, Modifiers::None },
    { 57413, Key::KeyPadAdd, Modifiers::None },
    { 57414, Key::KeyPadEnter, Modifiers::None },
    { 57415, Key::KeyPadEqual, Modifiers::None },
    { 57416, Key::KeyPadSeparator, Modifiers::None },
    { 57417, Key::KeyPadLeft, Modifiers::None },
    { 57418, Key::KeyPadRight, Modifiers::None },
    { 57419, Key::KeyPadUp, Modifiers::None },
    { 57420, Key::KeyPadDown, Modifiers::None },
    { 57421, Key::KeyPadPageUp, Modifiers::None },
    { 57422, Key::KeyPadPageDown, Modifiers::None },
    { 57423, Key::KeyPadHome, Modifiers::None },
    { 57424, Key::KeyPadEnd, Modifiers::None },
    { 57425, Key::KeyPadInsert, Modifiers::None },
    { 57426, Key::KeyPadDelete, Modifiers::None },
    { 57427, Key::KeyPadBegin, Modifiers::None },
    { 57428, Key::MediaPlay, Modifiers::None },
    { 57429, Key::MediaPause, Modifiers::None },
    { 57430, Key::MediaPlayPause, Modifiers::None },
    { 57431, Key::MediaReverse, Modifiers::None },
    { 57432, Key::MediaStop, Modifiers::None },
    { 57433, Key::MediaFastForward, Modifiers::None },
    { 57434, Key::MediaRewind, Modifiers::None },
    { 57435, Key::MediaTrackNext, Modifiers::None },
    { 57436, Key::MediaTrackPrevious, Modifiers::None },
    { 57437, Key::MediaRecord, Modifiers::None },
    { 57438, Key::LowerVolume, Modifiers::None },
    { 57439, Key::RaiseVolume, Modifiers::None },
    { 57440, Key::MuteVolume, Modifiers::None },
    { 57441, Key::LeftShift, Modifiers::None },
    { 57442, Key::LeftControl, Modifiers::None },
    { 57443, Key::LeftAlt, Modifiers::None },
    { 57444, Key::LeftSuper, Modifiers::None },
    { 57445, Key::LeftHyper, Modifiers::None },
    { 57446, Key::LeftMeta, Modifiers::None },
    { 57447, Key::RightShift, Modifiers::None },
    { 57449, Key::RightControl, Modifiers::None },
    { 57449, Key::RightAlt, Modifiers::None },
    { 57450, Key::RightSuper, Modifiers::None },
    { 57451, Key::RightHyper, Modifiers::None },
    { 57452, Key::RightMeta, Modifiers::None },
    { 57453, Key::IsoLevel3Shift, Modifiers::None },
    { 57454, Key::IsoLevel5Shift, Modifiers::None },
});

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
        emit(KeyEvent::key_down(Key::Escape));
        m_state = State::Base;
    }
    return di::move(m_pending_events);
}

auto TerminalInputParser::key_event_from_legacy_code_point(c32 code_point, Modifiers base_modifiers) -> KeyEvent {
    for (auto const& mapping : legacy_code_point_mappings) {
        if (mapping.code_point == code_point) {
            auto text = ""_s;
            if (mapping.code_point >= 32 && mapping.code_point < 127) {
                text = di::single(code_point) | di::to<di::String>();
            }
            return KeyEvent::key_down(mapping.key, di::move(text), mapping.modifiers | base_modifiers);
        }
    }

    // Otherwise, consider this to be a unicode text key event.
    auto text = di::single(code_point) | di::to<di::String>();
    return KeyEvent::key_down(Key::None, di::move(text), base_modifiers);
}

auto TerminalInputParser::key_event_from_ss3_code_point(c32 code_point, Modifiers base_modifiers)
    -> di::Optional<KeyEvent> {
    for (auto const& mapping : ss3_mappings) {
        if (code_point == mapping.code_point) {
            return KeyEvent::key_down(mapping.key, {}, base_modifiers | mapping.modifiers);
        }
    }
    return {};
}

auto TerminalInputParser::key_event_from_legacy_functional_key(c32 number, Modifiers base_modifiers)
    -> di::Optional<KeyEvent> {
    for (auto const& mapping : legacy_functional_key_mappings) {
        if (c32(number) == mapping.code_point) {
            return KeyEvent::key_down(mapping.key, {}, base_modifiers | mapping.modifiers);
        }
    }
    return {};
}

auto TerminalInputParser::key_event_from_code_point(c32 number, Modifiers base_modifiers, di::String text,
                                                    KeyEventType type) -> di::Optional<KeyEvent> {
    for (auto const& mapping : code_point_key_mappings) {
        if (c32(number) == mapping.code_point) {
            return KeyEvent(type, mapping.key, di::move(text), base_modifiers | mapping.modifiers);
        }
    }
    return {};
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

    // Parse the number list.
    auto nums = Params::from_string(m_accumulator);

    // In general, the key event will look something like this:
    //   code_point;modifiers
    // The code_point is defaulted to 1, while the modifiers are defaulted
    // to 0.
    auto code_point = U'\01';
    if (!nums.empty()) {
        code_point = nums.get(0, code_point);
    }
    auto modifiers = Modifiers::None;
    if (nums.size() >= 2) {
        modifiers = Modifiers(nums.get(1, u32(modifiers) + 1) - 1);
    }

    if (input == U'u') {
        // Use standard mappings if we end with a 'u'.
        //
        // We also need to parse the event type and text when using extended modes.
        auto type = KeyEventType::Press;
        if (nums.size() >= 2) {
            type = KeyEventType(nums.get_subparam(1, 1, u32(type)));
            if (!di::valid_enum_value(type)) {
                type = KeyEventType::Press;
            }
        }
        auto text = di::String {};
        if (nums.size() >= 3 && nums.get_subparam(2, 0, 0) != 0) {
            auto subparams = nums.subparams(2);
            for (auto i : di::range(subparams.size())) {
                text.push_back(c32(subparams.get(i)));
            }
        }
        key_event_from_code_point(code_point, modifiers, di::move(text), type)
            .transform(di::bind_front(&TerminalInputParser::emit, this));
    } else if (input == U'~') {
        // Use legacy mappings if we end with a '~'.
        key_event_from_legacy_functional_key(code_point, modifiers)
            .transform(di::bind_front(&TerminalInputParser::emit, this));
    } else {
        // Use SS3 mappings if we didn't end in 'u' or '~'.
        key_event_from_ss3_code_point(input, modifiers).transform(di::bind_front(&TerminalInputParser::emit, this));
    }
    m_state = State::Base;
}

void TerminalInputParser::handle_ss3(c32 input) {
    key_event_from_ss3_code_point(input).transform(di::bind_front(&TerminalInputParser::emit, this));
    m_state = State::Base;
}
}
