#include "di/vocab/array/array.h"
#include "dius/test/prelude.h"
#include "ttx/key.h"
#include "ttx/key_event.h"
#include "ttx/key_event_io.h"
#include "ttx/params.h"

namespace key_event_io {
static void serialize() {
    using namespace ttx;

    struct Case {
        KeyEvent event;
        di::Optional<di::StringView> expected {};
        KeyReportingFlags flags { KeyReportingFlags::None };
        ApplicationCursorKeysMode mode { ApplicationCursorKeysMode::Disabled };
    };

    auto cases = di::Array {
        // Application cursor keys
        Case { KeyEvent::key_down(Key::Down), "\033[B"_sv },
        Case { KeyEvent::key_down(Key::Down), "\033OB"_sv, KeyReportingFlags::None,
               ApplicationCursorKeysMode::Enabled },

        // Lock modifiers are ignored in legacy mode.
        Case { KeyEvent::key_down(Key::Home, {}, Modifiers::NumLock), "\033[H"_sv },
        Case { KeyEvent::key_down(Key::Home, {}, Modifiers::NumLock), "\033[1;129H"_sv,
               KeyReportingFlags::Disambiguate },

        // Special case for Enter
        Case { KeyEvent::key_down(Key::Enter), "\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Control), "\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Shift), "\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Control | Modifiers::Shift), "\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {},
                                  Modifiers::Control | Modifiers::Shift | Modifiers::CapsLock | Modifiers::NumLock),
               "\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Alt), "\033\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Alt | Modifiers::Control), "\033\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Alt | Modifiers::Shift), "\033\r"_sv },
        Case { KeyEvent::key_down(Key::Enter, {}, Modifiers::Alt | Modifiers::Control | Modifiers::Shift),
               "\033\r"_sv },
        Case { KeyEvent::key_down(Key::Enter), "\r"_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::Enter), "\033[13u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Special case for Escape
        Case { KeyEvent::key_down(Key::Escape), "\033"_sv },
        Case { KeyEvent::key_down(Key::Escape, {}, Modifiers::Control), "\033"_sv },
        Case { KeyEvent::key_down(Key::Escape, {}, Modifiers::Shift), "\033"_sv },
        Case { KeyEvent::key_down(Key::Escape, {}, Modifiers::Alt | Modifiers::Control | Modifiers::Shift),
               "\033\033"_sv },
        Case { KeyEvent::key_down(Key::Escape), "\033[27u"_sv, KeyReportingFlags::Disambiguate },

        // Special case for Backspace
        Case { KeyEvent::key_down(Key::Backspace), "\x7f"_sv },
        Case { KeyEvent::key_down(Key::Backspace, {}, Modifiers::Control), "\x08"_sv },
        Case { KeyEvent::key_down(Key::Backspace, {}, Modifiers::Shift), "\x7f"_sv },
        Case { KeyEvent::key_down(Key::Backspace, {}, Modifiers::Alt | Modifiers::Control | Modifiers::Shift),
               "\033\x08"_sv },
        Case { KeyEvent::key_down(Key::Backspace), "\x7f"_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::Backspace), "\033[127u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Special case for Tab
        Case { KeyEvent::key_down(Key::Tab), "\t"_sv },
        Case { KeyEvent::key_down(Key::Tab, {}, Modifiers::Control), "\t"_sv },
        Case { KeyEvent::key_down(Key::Tab, {}, Modifiers::Shift), "\033[Z"_sv },
        Case { KeyEvent::key_down(Key::Tab, {}, Modifiers::Alt | Modifiers::Control | Modifiers::Shift),
               "\033\033[Z"_sv },
        Case { KeyEvent::key_down(Key::Tab, {},
                                  Modifiers::Alt | Modifiers::Control | Modifiers::Shift | Modifiers::CapsLock),
               "\033\033[Z"_sv },
        Case { KeyEvent::key_down(Key::Tab), "\t"_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::Tab), "\033[9u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent::key_down(Key::Tab, {}, Modifiers::Shift | Modifiers::Meta), "\033[9;34u"_sv },

        // Special case for Space
        Case { KeyEvent::key_down(Key::Space, " "_s), " "_sv },
        Case { KeyEvent::key_down(Key::Space, {}, Modifiers::Control), "\x00"_sv },
        Case { KeyEvent::key_down(Key::Space, {}, Modifiers::Shift), " "_sv },
        Case { KeyEvent::key_down(Key::Space, {}, Modifiers::Alt | Modifiers::Control | Modifiers::Shift),
               "\033\x00"_sv },
        Case { KeyEvent::key_down(Key::Space, " "_s), " "_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::Space, " "_s), "\033[32u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Special case for Menu
        Case { KeyEvent::key_down(Key::Menu), "\033[29~"_sv },
        Case { KeyEvent::key_down(Key::Menu), "\033[57363u"_sv, KeyReportingFlags::Disambiguate },

        // Special case for F3
        Case { KeyEvent::key_down(Key::F3), "\033[R"_sv },
        Case { KeyEvent::key_down(Key::F3), "\033[13~"_sv, KeyReportingFlags::Disambiguate },

        // Shifted text keys
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift), "A"_sv },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift), "\033[97;2u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift), "\033[97;2;65u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes |
                   KeyReportingFlags::ReportAssociatedText },
        Case { KeyEvent::key_down(Key::A, {}, Modifiers::Shift | Modifiers::Alt), "\033A"_sv },
        Case { KeyEvent::key_down(Key::A, {}, Modifiers::Shift | Modifiers::Alt), "\033[97;4u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Control text keys
        Case { KeyEvent::key_down(Key::A, {}, Modifiers::Control), "\x01"_sv },
        Case { KeyEvent::key_down(Key::A, {}, Modifiers::Control), "\033[97;5u"_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::I, {}, Modifiers::Control), "\x09"_sv },
        Case { KeyEvent::key_down(Key::_6, {}, Modifiers::Control | Modifiers::Shift), "\x1e"_sv },
        Case { KeyEvent::key_down(Key::_0, {}, Modifiers::Control), "0"_sv },
        Case { KeyEvent::key_down(Key::_1, {}, Modifiers::Control), "1"_sv },
        Case { KeyEvent::key_down(Key::_9, {}, Modifiers::Control), "9"_sv },
        Case { KeyEvent::key_down(Key::_5, {}, Modifiers::Control | Modifiers::Shift), "\033[53;6u"_sv },
        Case { KeyEvent::key_down(Key::Quote, {}, Modifiers::Control), "'"_sv },

        // Event types
        Case { KeyEvent(KeyEventType::Release, Key::A), di::nullopt },
        Case { KeyEvent(KeyEventType::Release, Key::A), "\033[97;1:3u"_sv, KeyReportingFlags::ReportEventTypes },
        Case { KeyEvent(KeyEventType::Repeat, Key::A, "a"_s), "a"_sv },
        Case { KeyEvent(KeyEventType::Repeat, Key::A, "a"_s), "a"_sv, KeyReportingFlags::ReportEventTypes },
        Case { KeyEvent(KeyEventType::Repeat, Key::A, "a"_s), "\033[97;1:2u"_sv,
               KeyReportingFlags::ReportEventTypes | KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent(KeyEventType::Repeat, Key::Escape), "\033"_sv },
        Case { KeyEvent(KeyEventType::Repeat, Key::Escape), "\033"_sv, KeyReportingFlags::ReportEventTypes },
        Case { KeyEvent(KeyEventType::Repeat, Key::Escape), "\033[27;1:2u"_sv,
               KeyReportingFlags::ReportEventTypes | KeyReportingFlags::Disambiguate },

        // Alternate key reporting
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, U'A'), "A"_sv,
               KeyReportingFlags::ReportAlternateKeys },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, U'A'), "\033[97:65;2u"_sv,
               KeyReportingFlags::ReportAlternateKeys | KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, U'A'), "\033[97:65;2;65u"_sv,
               KeyReportingFlags::ReportAlternateKeys | KeyReportingFlags::ReportAllKeysAsEscapeCodes |
                   KeyReportingFlags::ReportAssociatedText },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, U'A', 99), "\033[97:65:99;2u"_sv,
               KeyReportingFlags::ReportAlternateKeys | KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, 0, 99), "\033[97::99;2u"_sv,
               KeyReportingFlags::ReportAlternateKeys | KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Keypad keys
        Case { KeyEvent::key_down(Key::KeyPadEnter), "\r"_sv },
        Case { KeyEvent::key_down(Key::KeyPadEnter), "\033[57414u"_sv, KeyReportingFlags::Disambiguate },
        Case { KeyEvent::key_down(Key::KeyPadMultiply), "*"_sv },
        Case { KeyEvent::key_down(Key::KeyPadMultiply, {}, Modifiers::Shift), "*"_sv },
        Case { KeyEvent::key_down(Key::KeyPadMultiply, {}, Modifiers::Control), "*"_sv },
        Case { KeyEvent::key_down(Key::KeyPadSubtract), "-"_sv },
        Case { KeyEvent::key_down(Key::KeyPadSubtract, {}, Modifiers::Shift), "-"_sv },
        Case { KeyEvent::key_down(Key::KeyPadSubtract, {}, Modifiers::Control), "-"_sv },
        Case { KeyEvent::key_down(Key::KeyPadDivide), "/"_sv },
        Case { KeyEvent::key_down(Key::KeyPadDivide, {}, Modifiers::Shift), "/"_sv },
        Case { KeyEvent::key_down(Key::KeyPadDivide, {}, Modifiers::Control), "\x1F"_sv },
        Case { KeyEvent::key_down(Key::KeyPadAdd), "+"_sv },
        Case { KeyEvent::key_down(Key::KeyPadAdd, {}, Modifiers::Shift), "+"_sv },
        Case { KeyEvent::key_down(Key::KeyPadAdd, {}, Modifiers::Alt | Modifiers::Shift), "\033+"_sv },
        Case { KeyEvent::key_down(Key::KeyPadAdd, {}, Modifiers::Alt | Modifiers::Control), "\033+"_sv },
        Case { KeyEvent::key_down(Key::KeyPadAdd, {}, Modifiers::Control | Modifiers::Shift), "\033[43;6u"_sv },

        // Modifiers/Lock keys
        Case { KeyEvent::key_down(Key::CapsLock), {} },
        Case { KeyEvent::key_down(Key::CapsLock), "\033[57358u"_sv, KeyReportingFlags::ReportAllKeysAsEscapeCodes },
        Case { KeyEvent::key_down(Key::LeftShift), {} },
        Case { KeyEvent::key_down(Key::LeftShift), "\033[57441u"_sv, KeyReportingFlags::ReportAllKeysAsEscapeCodes },

        // Text only events
        Case { KeyEvent::key_down(Key::None, u8"a\u0300"_s), u8"a\u0300"_sv },
        Case { KeyEvent::key_down(Key::None, u8"a\u0300"_s), u8"\033[0;;97:768u"_sv,
               KeyReportingFlags::Disambiguate | KeyReportingFlags::ReportAllKeysAsEscapeCodes |
                   KeyReportingFlags::ReportAssociatedText },
    };

    for (auto const& test_case : cases) {
        auto result = serialize_key_event(test_case.event, test_case.mode, test_case.flags);
        ASSERT_EQ(test_case.expected, result);
    }
}

static void parse() {
    using namespace ttx;

    struct Case {
        Params params {};
        c32 terminator { U'0' };
        di::Optional<KeyEvent> expected {};
    };

    auto cases = di::Array {
        // Tilde
        Case { { { 7 } }, '~', KeyEvent::key_down(Key::Home) },
        Case {
            { { 7, 12, 2 }, { 2, 3 } }, '~', KeyEvent(KeyEventType::Release, Key::Home, {}, Modifiers::Shift, 12, 2) },
        Case {
            { { 7, {}, 2 }, { 2, 3 } }, '~', KeyEvent(KeyEventType::Release, Key::Home, {}, Modifiers::Shift, 0, 2) },

        // Special
        Case { {}, 'Z', KeyEvent::key_down(Key::Tab, {}, Modifiers::Shift) },

        // Kitty
        Case { { { 97, 65, 99 }, { 2 }, { 65 } }, 'u', KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift, 65, 99) },

        // Errors
        Case {},
        Case { {}, 'Y' },
    };

    for (auto const& test_case : cases) {
        auto result = key_event_from_csi(test_case.params, test_case.terminator);
        ASSERT_EQ(test_case.expected, result);
    }
}

TEST(key_event_io, serialize)
TEST(key_event_io, parse)
}
