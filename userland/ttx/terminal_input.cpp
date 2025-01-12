#include "terminal_input.h"

#include "di/container/view/split.h"
#include "di/container/view/transform.h"
#include "di/reflect/valid_enum_value.h"
#include "di/vocab/array/to_array.h"
#include "key.h"
#include "key_event.h"

namespace ttx {
struct CodePointMapping {
    c32 code_point { 0 };
    Key key { Key::None };
    Modifers modifiers = Modifers::None;
};

constexpr auto legacy_code_point_mappings = di::to_array<CodePointMapping>({
    { 0x00, Key::Space, Modifers::Control },
    { 0x01, Key::A, Modifers::Control },
    { 0x02, Key::B, Modifers::Control },
    { 0x03, Key::C, Modifers::Control },
    { 0x04, Key::D, Modifers::Control },
    { 0x05, Key::E, Modifers::Control },
    { 0x06, Key::F, Modifers::Control },
    { 0x07, Key::G, Modifers::Control },
    { 0x08, Key::Backspace, Modifers::Control },
    { 0x09, Key::Tab, Modifers::None },
    { 0x0a, Key::J, Modifers::Control },
    { 0x0b, Key::K, Modifers::Control },
    { 0x0c, Key::L, Modifers::Control },
    { 0x0d, Key::Enter, Modifers::Control },
    { 0x0e, Key::N, Modifers::Control },
    { 0x0f, Key::O, Modifers::Control },
    { 0x10, Key::P, Modifers::Control },
    { 0x11, Key::Q, Modifers::Control },
    { 0x12, Key::R, Modifers::Control },
    { 0x13, Key::S, Modifers::Control },
    { 0x14, Key::T, Modifers::Control },
    { 0x15, Key::U, Modifers::Control },
    { 0x16, Key::V, Modifers::Control },
    { 0x17, Key::W, Modifers::Control },
    { 0x18, Key::X, Modifers::Control },
    { 0x19, Key::Y, Modifers::Control },
    { 0x1a, Key::Z, Modifers::Control },
    { 0x1b, Key::Escape, Modifers::None },
    { 0x1c, Key::BackSlash, Modifers::Control },
    { 0x1d, Key::RightBracket, Modifers::Control },
    { 0x1e, Key::_6, Modifers::Control },
    { 0x1f, Key::_7, Modifers::Control },

    { ' ', Key::Space, Modifers::None },
    { '!', Key::_1, Modifers::Shift },
    { '"', Key::Quote, Modifers::Shift },
    { '#', Key::_3, Modifers::Shift },
    { '$', Key::_4, Modifers::Shift },
    { '%', Key::_5, Modifers::Shift },
    { '&', Key::_7, Modifers::Shift },
    { '\'', Key::Quote, Modifers::None },
    { '(', Key::_9, Modifers::Shift },
    { ')', Key::_0, Modifers::Shift },
    { '*', Key::_8, Modifers::Shift },
    { '+', Key::Equal, Modifers::Shift },
    { ',', Key::Comma, Modifers::None },
    { '-', Key::Minus, Modifers::None },
    { '.', Key::D, Modifers::None },
    { '/', Key::Slash, Modifers::None },
    { '0', Key::_0, Modifers::None },
    { '1', Key::_1, Modifers::None },
    { '2', Key::_2, Modifers::None },
    { '3', Key::_3, Modifers::None },
    { '4', Key::_4, Modifers::None },
    { '5', Key::_5, Modifers::None },
    { '6', Key::_6, Modifers::None },
    { '7', Key::_7, Modifers::None },
    { '8', Key::_8, Modifers::None },
    { '9', Key::_9, Modifers::None },
    { ':', Key::SemiColon, Modifers::Shift },
    { ';', Key::SemiColon, Modifers::None },
    { '<', Key::Comma, Modifers::Shift },
    { '=', Key::Equal, Modifers::None },
    { '>', Key::Period, Modifers::Shift },
    { '?', Key::Slash, Modifers::Shift },
    { '@', Key::_2, Modifers::Shift },
    { 'A', Key::A, Modifers::Shift },
    { 'B', Key::B, Modifers::Shift },
    { 'C', Key::C, Modifers::Shift },
    { 'D', Key::D, Modifers::Shift },
    { 'E', Key::E, Modifers::Shift },
    { 'F', Key::F, Modifers::Shift },
    { 'G', Key::G, Modifers::Shift },
    { 'H', Key::H, Modifers::Shift },
    { 'I', Key::I, Modifers::Shift },
    { 'J', Key::J, Modifers::Shift },
    { 'K', Key::K, Modifers::Shift },
    { 'L', Key::L, Modifers::Shift },
    { 'M', Key::M, Modifers::Shift },
    { 'N', Key::N, Modifers::Shift },
    { 'O', Key::O, Modifers::Shift },
    { 'P', Key::P, Modifers::Shift },
    { 'Q', Key::Q, Modifers::Shift },
    { 'R', Key::R, Modifers::Shift },
    { 'S', Key::S, Modifers::Shift },
    { 'T', Key::T, Modifers::Shift },
    { 'U', Key::U, Modifers::Shift },
    { 'V', Key::V, Modifers::Shift },
    { 'W', Key::W, Modifers::Shift },
    { 'X', Key::X, Modifers::Shift },
    { 'Y', Key::Y, Modifers::Shift },
    { 'Z', Key::Z, Modifers::Shift },
    { '[', Key::LeftBracket, Modifers::None },
    { '\\', Key::BackSlash, Modifers::None },
    { ']', Key::RightBracket, Modifers::None },
    { '^', Key::_6, Modifers::Shift },
    { '_', Key::Minus, Modifers::Shift },
    { '`', Key::Backtick, Modifers::None },
    { 'a', Key::A, Modifers::None },
    { 'b', Key::B, Modifers::None },
    { 'c', Key::C, Modifers::None },
    { 'd', Key::D, Modifers::None },
    { 'e', Key::E, Modifers::None },
    { 'f', Key::F, Modifers::None },
    { 'g', Key::G, Modifers::None },
    { 'h', Key::H, Modifers::None },
    { 'i', Key::I, Modifers::None },
    { 'j', Key::J, Modifers::None },
    { 'k', Key::K, Modifers::None },
    { 'l', Key::L, Modifers::None },
    { 'm', Key::M, Modifers::None },
    { 'n', Key::N, Modifers::None },
    { 'o', Key::O, Modifers::None },
    { 'p', Key::P, Modifers::None },
    { 'q', Key::Q, Modifers::None },
    { 'r', Key::R, Modifers::None },
    { 's', Key::S, Modifers::None },
    { 't', Key::T, Modifers::None },
    { 'u', Key::U, Modifers::None },
    { 'v', Key::V, Modifers::None },
    { 'w', Key::W, Modifers::None },
    { 'x', Key::X, Modifers::None },
    { 'y', Key::Y, Modifers::None },
    { 'z', Key::Z, Modifers::None },
    { '{', Key::LeftBracket, Modifers::Shift },
    { '|', Key::BackSlash, Modifers::Shift },
    { '}', Key::RightBracket, Modifers::Shift },
    { '~', Key::Backtick, Modifers::Shift },

    { 0x7f, Key::Backspace, Modifers::None },
});

constexpr auto ss3_mappings = di::to_array<CodePointMapping>({
    { 'A', Key::Up, Modifers::None },
    { 'B', Key::Down, Modifers::None },
    { 'C', Key::Right, Modifers::None },
    { 'D', Key::Left, Modifers::None },
    { 'E', Key::KeyPadBegin, Modifers::None },
    { 'H', Key::Home, Modifers::None },
    { 'F', Key::End, Modifers::None },
    { 'P', Key::F1, Modifers::None },
    { 'Q', Key::F2, Modifers::None },
    { 'R', Key::F3, Modifers::None },
    { 'S', Key::F4, Modifers::None },
});

constexpr auto legacy_functional_key_mappings = di::to_array<CodePointMapping>({
    { 2, Key::Insert, Modifers::None },   { 3, Key::Delete, Modifers::None }, { 5, Key::PageUp, Modifers::None },
    { 6, Key::PageDown, Modifers::None }, { 7, Key::Home, Modifers::None },   { 8, Key::End, Modifers::None },
    { 11, Key::F1, Modifers::None },      { 12, Key::F2, Modifers::None },    { 13, Key::F3, Modifers::None },
    { 14, Key::F4, Modifers::None },      { 15, Key::F5, Modifers::None },    { 17, Key::F6, Modifers::None },
    { 18, Key::F7, Modifers::None },      { 19, Key::F8, Modifers::None },    { 20, Key::F9, Modifers::None },
    { 21, Key::F10, Modifers::None },     { 23, Key::F11, Modifers::None },   { 24, Key::F12, Modifers::None },
    { 29, Key::Menu, Modifers::None },
});

constexpr auto code_point_key_mappings = di::to_array<CodePointMapping>({
    { 9, Key::Tab, Modifers::None },
    { 13, Key::Enter, Modifers::None },
    { 27, Key::Escape, Modifers::None },
    { ' ', Key::Space, Modifers::None },
    { '\'', Key::Quote, Modifers::None },
    { ',', Key::Comma, Modifers::None },
    { '-', Key::Minus, Modifers::None },
    { '.', Key::D, Modifers::None },
    { '/', Key::Slash, Modifers::None },
    { '0', Key::_0, Modifers::None },
    { '1', Key::_1, Modifers::None },
    { '2', Key::_2, Modifers::None },
    { '3', Key::_3, Modifers::None },
    { '4', Key::_4, Modifers::None },
    { '5', Key::_5, Modifers::None },
    { '6', Key::_6, Modifers::None },
    { '7', Key::_7, Modifers::None },
    { '8', Key::_8, Modifers::None },
    { '9', Key::_9, Modifers::None },
    { ';', Key::SemiColon, Modifers::None },
    { '=', Key::Equal, Modifers::None },
    { '[', Key::LeftBracket, Modifers::None },
    { '\\', Key::BackSlash, Modifers::None },
    { ']', Key::RightBracket, Modifers::None },
    { '`', Key::Backtick, Modifers::None },
    { 'a', Key::A, Modifers::None },
    { 'b', Key::B, Modifers::None },
    { 'c', Key::C, Modifers::None },
    { 'd', Key::D, Modifers::None },
    { 'e', Key::E, Modifers::None },
    { 'f', Key::F, Modifers::None },
    { 'g', Key::G, Modifers::None },
    { 'h', Key::H, Modifers::None },
    { 'i', Key::I, Modifers::None },
    { 'j', Key::J, Modifers::None },
    { 'k', Key::K, Modifers::None },
    { 'l', Key::L, Modifers::None },
    { 'm', Key::M, Modifers::None },
    { 'n', Key::N, Modifers::None },
    { 'o', Key::O, Modifers::None },
    { 'p', Key::P, Modifers::None },
    { 'q', Key::Q, Modifers::None },
    { 'r', Key::R, Modifers::None },
    { 's', Key::S, Modifers::None },
    { 't', Key::T, Modifers::None },
    { 'u', Key::U, Modifers::None },
    { 'v', Key::V, Modifers::None },
    { 'w', Key::W, Modifers::None },
    { 'x', Key::X, Modifers::None },
    { 'y', Key::Y, Modifers::None },
    { 'z', Key::Z, Modifers::None },
    { 127, Key::Backspace, Modifers::None },

    { 57358, Key::CapsLock, Modifers::None },
    { 57359, Key::ScrollLock, Modifers::None },
    { 57360, Key::NumLock, Modifers::None },
    { 57361, Key::PrintScreen, Modifers::None },
    { 57362, Key::Pause, Modifers::None },
    { 57363, Key::Menu, Modifers::None },
    { 57376, Key::F13, Modifers::None },
    { 57377, Key::F14, Modifers::None },
    { 57378, Key::F15, Modifers::None },
    { 57379, Key::F16, Modifers::None },
    { 57380, Key::F17, Modifers::None },
    { 57381, Key::F18, Modifers::None },
    { 57382, Key::F19, Modifers::None },
    { 57383, Key::F20, Modifers::None },
    { 57384, Key::F21, Modifers::None },
    { 57385, Key::F22, Modifers::None },
    { 57386, Key::F23, Modifers::None },
    { 57387, Key::F24, Modifers::None },
    { 57388, Key::F25, Modifers::None },
    { 57389, Key::F26, Modifers::None },
    { 57390, Key::F27, Modifers::None },
    { 57391, Key::F28, Modifers::None },
    { 57392, Key::F29, Modifers::None },
    { 57393, Key::F30, Modifers::None },
    { 57394, Key::F31, Modifers::None },
    { 57395, Key::F32, Modifers::None },
    { 57396, Key::F33, Modifers::None },
    { 57397, Key::F34, Modifers::None },
    { 57398, Key::F13, Modifers::None },
    { 57399, Key::KeyPad0, Modifers::None },
    { 57400, Key::KeyPad1, Modifers::None },
    { 57401, Key::KeyPad2, Modifers::None },
    { 57402, Key::KeyPad3, Modifers::None },
    { 57403, Key::KeyPad4, Modifers::None },
    { 57404, Key::KeyPad5, Modifers::None },
    { 57405, Key::KeyPad6, Modifers::None },
    { 57406, Key::KeyPad7, Modifers::None },
    { 57407, Key::KeyPad8, Modifers::None },
    { 57408, Key::KeyPad9, Modifers::None },
    { 57409, Key::KeyPadDecimal, Modifers::None },
    { 57410, Key::KeyPadDivide, Modifers::None },
    { 57411, Key::KeyPadMultiply, Modifers::None },
    { 57412, Key::KeyPadSubtract, Modifers::None },
    { 57413, Key::KeyPadAdd, Modifers::None },
    { 57414, Key::KeyPadEnter, Modifers::None },
    { 57415, Key::KeyPadEqual, Modifers::None },
    { 57416, Key::KeyPadSeparator, Modifers::None },
    { 57417, Key::KeyPadLeft, Modifers::None },
    { 57418, Key::KeyPadRight, Modifers::None },
    { 57419, Key::KeyPadUp, Modifers::None },
    { 57420, Key::KeyPadDown, Modifers::None },
    { 57421, Key::KeyPadPageUp, Modifers::None },
    { 57422, Key::KeyPadPageDown, Modifers::None },
    { 57423, Key::KeyPadHome, Modifers::None },
    { 57424, Key::KeyPadEnd, Modifers::None },
    { 57425, Key::KeyPadInsert, Modifers::None },
    { 57426, Key::KeyPadDelete, Modifers::None },
    { 57427, Key::KeyPadBegin, Modifers::None },
    { 57428, Key::MediaPlay, Modifers::None },
    { 57429, Key::MediaPause, Modifers::None },
    { 57430, Key::MediaPlayPause, Modifers::None },
    { 57431, Key::MediaReverse, Modifers::None },
    { 57432, Key::MediaStop, Modifers::None },
    { 57433, Key::MediaFastForward, Modifers::None },
    { 57434, Key::MediaRewind, Modifers::None },
    { 57435, Key::MediaTrackNext, Modifers::None },
    { 57436, Key::MediaTrackPrevious, Modifers::None },
    { 57437, Key::MediaRecord, Modifers::None },
    { 57438, Key::LowerVolume, Modifers::None },
    { 57439, Key::RaiseVolume, Modifers::None },
    { 57440, Key::MuteVolume, Modifers::None },
    { 57441, Key::LeftShift, Modifers::None },
    { 57442, Key::LeftControl, Modifers::None },
    { 57443, Key::LeftAlt, Modifers::None },
    { 57444, Key::LeftSuper, Modifers::None },
    { 57445, Key::LeftHyper, Modifers::None },
    { 57446, Key::LeftMeta, Modifers::None },
    { 57447, Key::RightShift, Modifers::None },
    { 57449, Key::RightControl, Modifers::None },
    { 57449, Key::RightAlt, Modifers::None },
    { 57450, Key::RightSuper, Modifers::None },
    { 57451, Key::RightHyper, Modifers::None },
    { 57452, Key::RightMeta, Modifers::None },
    { 57453, Key::IsoLevel3Shift, Modifers::None },
    { 57454, Key::IsoLevel5Shift, Modifers::None },
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

auto TerminalInputParser::key_event_from_legacy_code_point(c32 code_point, Modifers base_modifiers) -> KeyEvent {
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

auto TerminalInputParser::key_event_from_ss3_code_point(c32 code_point, Modifers base_modifiers)
    -> di::Optional<KeyEvent> {
    for (auto const& mapping : ss3_mappings) {
        if (code_point == mapping.code_point) {
            return KeyEvent::key_down(mapping.key, {}, base_modifiers | mapping.modifiers);
        }
    }
    return {};
}

auto TerminalInputParser::key_event_from_legacy_functional_key(c32 number, Modifers base_modifiers)
    -> di::Optional<KeyEvent> {
    for (auto const& mapping : legacy_functional_key_mappings) {
        if (c32(number) == mapping.code_point) {
            return KeyEvent::key_down(mapping.key, {}, base_modifiers | mapping.modifiers);
        }
    }
    return {};
}

auto TerminalInputParser::key_event_from_code_point(c32 number, Modifers base_modifiers, di::String text,
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
            emit(key_event_from_legacy_code_point(input, Modifers::Alt));
            m_state = State::Base;
            break;
    }
}

// Take a list of `;` + `:` separated numbers, and try to parse them. Invalid sequences will result in a value of 1.
//
// For instance, 1:2;4;;5 will be parsed as:
//   [
//     [1,2]
//     [4],
//     [],
//     [5],
//   ]
static auto parse_ansi_numbers(di::StringView view) -> di::Vector<di::Vector<i32>> {
    return view | di::split(U';') | di::transform([](di::StringView nums) -> di::Vector<i32> {
               return nums | di::split(U':') | di::transform([](di::StringView num) -> i32 {
                          return di::parse<i32>(num).value_or(1);
                      }) |
                      di::to<di::Vector>();
           }) |
           di::to<di::Vector>();
}

void TerminalInputParser::handle_csi(c32 input) {
    // Check if input does not end a key press.
    auto is_digit = input >= U'0' && input <= U'9';
    if (input == U';' || input == U':' || is_digit) {
        m_accumulator.push_back(input);
        return;
    }

    // Parse the number list.
    auto nums = parse_ansi_numbers(m_accumulator);

    // In general, the key event will look something like this:
    //   code_point;modifiers
    // The code_point is defaulted to 1, while the modifiers are defaulted
    // to 0.
    auto code_point = U'\01';
    if (!nums.empty()) {
        code_point = nums[0]
                         .front()
                         .transform([](i32 value) {
                             return static_cast<c32>(value);
                         })
                         .value_or(code_point);
    }
    auto modifiers = Modifers::None;
    if (nums.size() >= 2) {
        modifiers = nums[1]
                        .front()
                        .transform([](i32 value) {
                            return static_cast<Modifers>(value - 1);
                        })
                        .value_or(modifiers);
    }

    if (input == U'u') {
        // Use standard mappings if we end with a 'u'.
        //
        // We also need to parse the event type and text when using extended modes.
        auto type = KeyEventType::Press;
        if (nums.size() >= 2) {
            type = nums[1]
                       .at(1)
                       .transform([](i32 value) {
                           return static_cast<KeyEventType>(value);
                       })
                       .value_or(type);
            if (!di::valid_enum_value(type)) {
                type = KeyEventType::Press;
            }
        }
        auto text = di::String {};
        if (nums.size() >= 3 && !di::container::equal(nums[2], di::single(0))) {
            for (auto code_point : nums[2]) {
                text.push_back(c32(code_point));
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
