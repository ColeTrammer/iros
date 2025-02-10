#include "dius/test/prelude.h"
#include "ttx/focus_event.h"
#include "ttx/key_event.h"
#include "ttx/modifiers.h"
#include "ttx/mouse.h"
#include "ttx/mouse_event.h"
#include "ttx/paste_event.h"
#include "ttx/terminal_input.h"

namespace terminal_input {

static void keyboard() {
    using namespace ttx;

    constexpr auto input = "A\033A\033[A\033OA\0\033[97;;97u\33[3~\033\033\033"_sv;

    auto expected = di::Array {
        Event(KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift)),
        Event(KeyEvent::key_down(Key::A, ""_s, Modifiers::Shift | Modifiers::Alt)),
        Event(KeyEvent::key_down(Key::Up)),
        Event(KeyEvent::key_down(Key::Up)),
        Event(KeyEvent::key_down(Key::_2, ""_s, Modifiers::Control | Modifiers::Shift)),
        Event(KeyEvent::key_down(Key::A, "a"_s)),
        Event(KeyEvent::key_down(Key::Delete)),
        Event(KeyEvent::key_down(Key::LeftBracket, ""_s, Modifiers::Control | Modifiers::Alt)),
        Event(KeyEvent::key_down(Key::LeftBracket, ""_s, Modifiers::Control)),
    };

    auto parser = TerminalInputParser {};
    auto actual = parser.parse(input);

    for (auto const& [ex, ac] : di::zip(expected, actual)) {
        ASSERT_EQ(ex, ac);
    }
    ASSERT_EQ(expected.size(), actual.size());
}

static void mouse() {
    using namespace ttx;

    constexpr auto input = "\033[<0;1;1M"_sv;

    auto expected = di::Array {
        Event(MouseEvent::press(MouseButton::Left)),
    };

    auto parser = TerminalInputParser {};
    auto actual = parser.parse(input);

    for (auto const& [ex, ac] : di::zip(expected, actual)) {
        ASSERT_EQ(ex, ac);
    }
    ASSERT_EQ(expected.size(), actual.size());
}

static void focus() {
    using namespace ttx;

    constexpr auto input = "\033[I\033[O"_sv;

    auto expected = di::Array {
        Event(FocusEvent::focus_in()),
        Event(FocusEvent::focus_out()),
    };

    auto parser = TerminalInputParser {};
    auto actual = parser.parse(input);

    for (auto const& [ex, ac] : di::zip(expected, actual)) {
        ASSERT_EQ(ex, ac);
    }
    ASSERT_EQ(expected.size(), actual.size());
}

static void paste() {
    using namespace ttx;

    constexpr auto input = "\033[I\033[200~\033ABC\033[O\033[201~\033[OA\00"_sv;

    auto expected = di::Array {
        Event(FocusEvent::focus_in()),
        Event(PasteEvent("\033ABC\033[O"_s)),
        Event(FocusEvent::focus_out()),
        Event(KeyEvent::key_down(Key::A, "A"_s, Modifiers::Shift)),
        Event(KeyEvent::key_down(Key::_2, ""_s, Modifiers::Shift | Modifiers::Control)),
    };

    auto parser = TerminalInputParser {};
    auto actual = parser.parse(input);

    for (auto const& [ex, ac] : di::zip(expected, actual)) {
        ASSERT_EQ(ex, ac);
    }
    ASSERT_EQ(expected.size(), actual.size());
}

TEST(terminal_input, keyboard)
TEST(terminal_input, mouse)
TEST(terminal_input, focus)
TEST(terminal_input, paste)
}
