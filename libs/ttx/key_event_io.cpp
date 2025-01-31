#include "ttx/key_event_io.h"

#include "di/format/prelude.h"
#include "di/vocab/array/to_array.h"
#include "ttx/key.h"
#include "ttx/key_event.h"
#include "ttx/params.h"

namespace ttx {
struct CodePointMapping {
    c32 code_point { 0 };
    Key key { Key::None };
    Modifiers modifiers = Modifiers::None;
};

// NOTE: this table is layed out such that the first entry maps
// to the actual key event produced when parsing legacy keys.
//
// For control keys, the first entry is chosen based on the keys
// visual representation. So control+space = ^@ will be reported
// as control+shift+2 if the terminal we run in doesn't support
// the kitty key protocol.
constexpr auto legacy_code_point_mappings = di::to_array<CodePointMapping>({
    { 0x00, Key::_2, Modifiers::Control | Modifiers::Shift },
    { 0x00, Key::_2, Modifiers::Control },
    { 0x00, Key::Space, Modifiers::Control },
    { 0x00, Key::Space, Modifiers::Control | Modifiers::Shift },

    { 0x01, Key::A, Modifiers::Control },
    { 0x02, Key::B, Modifiers::Control },
    { 0x03, Key::C, Modifiers::Control },
    { 0x04, Key::D, Modifiers::Control },
    { 0x05, Key::E, Modifiers::Control },
    { 0x06, Key::F, Modifiers::Control },
    { 0x07, Key::G, Modifiers::Control },

    { 0x08, Key::H, Modifiers::Control },
    { 0x08, Key::Backspace, Modifiers::Control },
    { 0x08, Key::Backspace, Modifiers::Control | Modifiers::Shift },

    { 0x09, Key::I, Modifiers::Control },
    { 0x09, Key::Tab, Modifiers::None },
    { 0x09, Key::Tab, Modifiers::Control },

    { 0x0a, Key::J, Modifiers::Control },
    { 0x0b, Key::K, Modifiers::Control },
    { 0x0c, Key::L, Modifiers::Control },

    { 0x0d, Key::M, Modifiers::Control },
    { 0x0d, Key::Enter, Modifiers::None },
    { 0x0d, Key::Enter, Modifiers::Shift },
    { 0x0d, Key::Enter, Modifiers::Control },
    { 0x0d, Key::Enter, Modifiers::Control | Modifiers::Shift },

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

    { 0x1b, Key::LeftBracket, Modifiers::None },
    { 0x1b, Key::_3, Modifiers::None },
    { 0x1b, Key::Escape, Modifiers::None },
    { 0x1b, Key::Escape, Modifiers::Control },
    { 0x1b, Key::Escape, Modifiers::Shift },
    { 0x1b, Key::Escape, Modifiers::Control | Modifiers::Shift },

    { 0x1c, Key::BackSlash, Modifiers::Control },
    { 0x1c, Key::_4, Modifiers::Control },

    { 0x1d, Key::RightBracket, Modifiers::Control },
    { 0x1d, Key::_5, Modifiers::Control },

    { 0x1e, Key::_6, Modifiers::Control | Modifiers::Shift },
    { 0x1e, Key::_6, Modifiers::Control },
    { 0x1e, Key::Backtick, Modifiers::Control | Modifiers::Shift },

    { 0x1f, Key::Minus, Modifiers::Control | Modifiers::Shift },
    { 0x1f, Key::Slash, Modifiers::Control },
    { 0x1f, Key::_7, Modifiers::Control },

    { ' ', Key::Space, Modifiers::None },
    { ' ', Key::Space, Modifiers::Shift },
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
    { '*', Key::Star, Modifiers::None },
    { '+', Key::Equal, Modifiers::Shift },
    { '+', Key::Plus, Modifiers::None },
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
    { '9', Key::_9, Modifiers::Control },
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

    { 0x7f, Key::Slash, Modifiers::Control | Modifiers::Shift },
    { 0x7f, Key::_7, Modifiers::Control },
    { 0x7f, Key::Backspace, Modifiers::None },
    { 0x7f, Key::Backspace, Modifiers::Shift },
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
    { 'Z', Key::Tab, Modifiers::Shift }, // CSI Z is a special case for shift+tab.
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
    { 0, Key::None, Modifiers::None },
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

    { '*', Key::Star, Modifiers::None },
    { '+', Key::Plus, Modifiers::None },

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
    { 57398, Key::F35, Modifiers::None },
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

constexpr auto numpad_key_mappings = di::to_array<di::Tuple<Key, Key>>({
    { Key::KeyPad0, Key::_0 },          { Key::KeyPad1, Key::_1 },           { Key::KeyPad2, Key::_2 },
    { Key::KeyPad3, Key::_3 },          { Key::KeyPad4, Key::_4 },           { Key::KeyPad5, Key::_5 },
    { Key::KeyPad6, Key::_6 },          { Key::KeyPad7, Key::_7 },           { Key::KeyPad8, Key::_8 },
    { Key::KeyPad9, Key::_9 },          { Key::KeyPadDecimal, Key::Period }, { Key::KeyPadDivide, Key::Slash },
    { Key::KeyPadMultiply, Key::Star }, { Key::KeyPadSubtract, Key::Minus }, { Key::KeyPadAdd, Key::Plus },
    { Key::KeyPadEnter, Key::Enter },   { Key::KeyPadEqual, Key::Equal },    { Key::KeyPadSeparator, Key::Comma },
    { Key::KeyPadLeft, Key::Left },     { Key::KeyPadRight, Key::Right },    { Key::KeyPadUp, Key::Up },
    { Key::KeyPadDown, Key::Down },     { Key::KeyPadPageUp, Key::PageUp },  { Key::KeyPadPageDown, Key::PageUp },
    { Key::KeyPadHome, Key::Home },     { Key::KeyPadEnd, Key::End },        { Key::KeyPadInsert, Key::Insert },
    { Key::KeyPadDelete, Key::Delete },
});

// Convert a key to a non-numpad version. For very uncommon keys,
// this may leave the key untouched (such as KeyPadBegin).
auto normalize_keypad_key(Key key) {
    for (auto [v, r] : numpad_key_mappings) {
        if (key == v) {
            return r;
        }
    }
    return key;
}

// These keys as reporting as code points even in the disambiguate mode, for compatibility reasons.
auto is_special_key_for_reporting(Key key) {
    return key == Key::Enter || key == Key::Tab || key == Key::Backspace;
}

auto make_key_event_string(u32 num, u32 modifiers, c32 ending_code_point, u32 event_type, u32 shifted_key,
                           u32 base_layout_key, di::StringView text, ApplicationCursorKeysMode cursor_key_mode) {
    // Special case: num=1 and modifiers=1 and event_type=1 and shifted_key=0 and base_layout_key=0 and text="".
    if (num == 1 && modifiers == 1 && event_type == 1 && shifted_key == 0 && base_layout_key == 0 && text.empty()) {
        // If in application cursor key mode, send SS3 code_point instead of CSI code_point.
        if (cursor_key_mode == ApplicationCursorKeysMode::Enabled) {
            return *di::present("\033O{}"_sv, ending_code_point);
        } else {
            return *di::present("\033[{}"_sv, ending_code_point);
        }
    }

    // Special case: modifiers=1 and event_type=1 and shifted=0 and base=0 and text="".
    if (modifiers == 1 && event_type == 1 && shifted_key == 0 && base_layout_key == 0 && text.empty()) {
        return *di::present("\033[{}{}"_sv, num, ending_code_point);
    }

    // Build up the paramaters. We'll end up with a string like:
    //   CSI num:shifted_key:base_layout_key;modifiers:event_type;text ending_code_point.
    auto params = Params({ { num } });
    if (shifted_key != 0) {
        params.add_subparam(shifted_key);
    }
    if (base_layout_key != 0) {
        // Add shifted key if needed.
        if (params.subparams(0).size() == 1) {
            params.add_empty_subparam();
        }
        params.add_subparam(base_layout_key);
    }

    if (modifiers != 1) {
        params.add_param(modifiers);
    }

    if (event_type != 1) {
        // Add modifiers if needed.
        if (params.size() == 1) {
            params.add_param(modifiers);
        }
        params.add_subparam(event_type);
    }

    if (!text.empty()) {
        // Add modifiers if needed.
        if (params.size() == 1) {
            params.add_empty_param();
        }
        params.add_subparams(text | di::transform([](c32 code_point) {
                                 return Param(code_point);
                             }) |
                             di::to<di::Vector>());
    }

    return *di::present("\033[{}{}"_sv, params, ending_code_point);
}

auto serialize_key_event(KeyEvent const& event, ApplicationCursorKeysMode cursor_key_mode, KeyReportingFlags flags)
    -> di::Optional<di::String> {
    // If there is text, and text does not send key events, then just send the text.
    if (!(flags & KeyReportingFlags::ReportAllKeysAsEscapeCodes) && !event.text().empty()) {
        return event.text().to_owned();
    }

    // Map keypad keys to regular keys if the disambiguate flag is not used.
    auto key = event.key();
    if (!(flags & KeyReportingFlags::Disambiguate)) {
        key = normalize_keypad_key(key);
    }

    // If we're not reporting key event types, release events are ignored. Release events are also ignored
    // for special keys when not reporting all keys as escape codes.
    if (event.type() == KeyEventType::Release &&
        (!(flags & KeyReportingFlags::ReportEventTypes) ||
         (!(flags & KeyReportingFlags::ReportAllKeysAsEscapeCodes) && is_special_key_for_reporting(key)))) {
        return {};
    }

    // Ignore modifiers and lock keys unless we're reporting all keys.
    if (((key > Key::ModifiersBegin && key < Key::ModifiersEnd) || key == Key::CapsLock || key == Key::ScrollLock ||
         key == Key::NumLock) &&
        !(flags & KeyReportingFlags::ReportAllKeysAsEscapeCodes)) {
        return {};
    }

    // Repeat events are reported as press events unless report event types is true.
    auto event_type = event.type() == KeyEventType::Repeat && !(flags & KeyReportingFlags::ReportEventTypes)
                          ? KeyEventType::Press
                          : event.type();

    // Check if we're should try to output a legacy single byte sequence.
    auto try_legacy_code_point =
        event.text().empty() && event_type != KeyEventType::Release &&
        (!(flags & KeyReportingFlags::Disambiguate) ||
         (is_special_key_for_reporting(key) && !(flags & KeyReportingFlags::ReportAllKeysAsEscapeCodes)));
    if (try_legacy_code_point) {
        auto has_alt = !!(event.modifiers() & Modifiers::Alt);
        auto modifiers = event.modifiers() & ~(Modifiers::Alt | Modifiers::LockModifiers);

        // Keypad special case: ignore shift is shift is the only modifier.
        if (key != event.key() && modifiers == Modifiers::Shift) {
            modifiers &= ~Modifiers::Shift;
        }

        // Tab special case: shift+tab outputs CSI Z.
        if (key == Key::Tab && !!(modifiers & Modifiers::Shift) &&
            !(modifiers & ~(Modifiers::Shift | Modifiers::Control))) {
            return *di::present("{}\x1b[Z"_sv, has_alt ? "\x1b"_sv : ""_sv);
        }

        for (auto const& mapping : legacy_code_point_mappings) {
            if (key == mapping.key && modifiers == mapping.modifiers) {
                return *di::present("{}{}"_sv, has_alt ? "\x1b"_sv : ""_sv, mapping.code_point);
            }
        }

        // Fallback case: if control is the only modifier, and we matched nothing, try again
        // without control. This ensures keys like `'` will be output unchanged when used with
        // control.
        if (modifiers == Modifiers::Control) {
            modifiers &= ~Modifiers::Control;

            for (auto const& mapping : legacy_code_point_mappings) {
                if (key == mapping.key && modifiers == mapping.modifiers) {
                    return *di::present("{}{}"_sv, has_alt ? "\x1b"_sv : ""_sv, mapping.code_point);
                }
            }
        }
    }

    // Modifiers are respented as the bit mask plus 1.
    // Lock modifiers are ignored in legacy mode.
    auto modifiers = 1 + static_cast<u32>(!(flags & KeyReportingFlags::Disambiguate)
                                              ? (event.modifiers() & ~Modifiers::LockModifiers)
                                              : event.modifiers());

    // Text isn't reported unless requested.
    auto text = !!(flags & KeyReportingFlags::ReportAssociatedText) ? event.text() : di::StringView {};

    // Alternate layout keys aren't reported unless requested.
    auto shifted_key = !!(flags & KeyReportingFlags::ReportAlternateKeys) ? u32(event.shifted_key()) : 0;
    auto base_layout_key = !!(flags & KeyReportingFlags::ReportAlternateKeys) ? u32(event.base_layout_key()) : 0;

    // Now, try and serialize the key event using the kitty protocol.
    // We start with the legacy mappings SS3 mappings (note Tab and F3 are special cases).
    if (key != Key::Tab && (key != Key::F3 || !(flags & KeyReportingFlags::Disambiguate))) {
        for (auto const& mapping : ss3_mappings) {
            if (key == mapping.key) {
                return make_key_event_string(1, modifiers, mapping.code_point, u32(event_type), shifted_key,
                                             base_layout_key, text, cursor_key_mode);
            }
        }
    }

    // Now use the legacy functional key mappings (CSI n ~).
    // Note, the Menu key is a special case.
    if (key != Key::Menu || !(flags & KeyReportingFlags::Disambiguate)) {
        for (auto const& mapping : legacy_functional_key_mappings) {
            if (key == mapping.key) {
                return make_key_event_string(mapping.code_point, modifiers, U'~', u32(event_type), shifted_key,
                                             base_layout_key, text, cursor_key_mode);
            }
        }
    }

    // Use the CSI u mappings.
    for (auto const& mapping : code_point_key_mappings) {
        if (key == mapping.key) {
            return make_key_event_string(mapping.code_point, modifiers, U'u', u32(event_type), shifted_key,
                                         base_layout_key, text, cursor_key_mode);
        }
    }

    return {};
}

auto key_event_from_legacy_code_point(c32 code_point, Modifiers base_modifiers) -> KeyEvent {
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

auto key_event_from_ss3_code_point(c32 code_point, u32 shifted_key, u32 base_layout_key, Modifiers base_modifiers,
                                   di::String text, KeyEventType type) -> di::Optional<KeyEvent> {
    for (auto const& mapping : ss3_mappings) {
        if (code_point == mapping.code_point) {
            return KeyEvent(type, mapping.key, di::move(text), base_modifiers | mapping.modifiers, shifted_key,
                            base_layout_key);
        }
    }
    return {};
}

auto key_event_from_legacy_functional_key(c32 number, u32 shifted_key, u32 base_layout_key, Modifiers base_modifiers,
                                          di::String text, KeyEventType type) -> di::Optional<KeyEvent> {
    for (auto const& mapping : legacy_functional_key_mappings) {
        if (c32(number) == mapping.code_point) {
            return KeyEvent(type, mapping.key, di::move(text), base_modifiers | mapping.modifiers, shifted_key,
                            base_layout_key);
        }
    }
    return {};
}

auto key_event_from_code_point(c32 number, u32 shifted_key, u32 base_layout_key, Modifiers base_modifiers,
                               di::String text, KeyEventType type) -> di::Optional<KeyEvent> {
    for (auto const& mapping : code_point_key_mappings) {
        if (c32(number) == mapping.code_point) {
            return KeyEvent(type, mapping.key, di::move(text), base_modifiers | mapping.modifiers, shifted_key,
                            base_layout_key);
        }
    }
    return {};
}

auto key_event_from_csi(Params const& params, c32 terminator) -> di::Optional<KeyEvent> {
    // In general, the key event will look something like this:
    //   num:shifted_key:base_layout_key;modifiers:event_type;text [ABCDEFHPQS~u]

    // The code_point is defaulted to 1, while the modifiers are defaulted
    // to 0.
    auto code_point = c32(params.get(0, 1));
    auto shifted_key = c32(params.get_subparam(0, 1));
    auto base_layout_key = c32(params.get_subparam(0, 2));
    auto modifiers = Modifiers(params.get(1, 1) - 1);
    auto type = KeyEventType(params.get_subparam(1, 1, u32(KeyEventType::Press)));
    if (!di::valid_enum_value(type)) {
        type = KeyEventType::Press;
    }

    auto text = di::String {};
    if (!params.subparams(2).empty()) {
        auto subparams = params.subparams(2);
        for (auto i : di::range(subparams.size())) {
            text.push_back(c32(subparams.get(i)));
        }
    }
    if (terminator == U'u') {
        return key_event_from_code_point(code_point, shifted_key, base_layout_key, modifiers, di::move(text), type);
    } else if (terminator == U'~') {
        // Use legacy mappings if we end with a '~'.
        return key_event_from_legacy_functional_key(code_point, shifted_key, base_layout_key, modifiers, di::move(text),
                                                    type);
    } else {
        // Use SS3 mappings if we didn't end in 'u' or '~'.
        return key_event_from_ss3_code_point(terminator, shifted_key, base_layout_key, modifiers, di::move(text), type);
    }
}
}
