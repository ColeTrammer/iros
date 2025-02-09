#include "di/vocab/array/array.h"
#include "dius/test/prelude.h"
#include "dius/tty.h"
#include "ttx/key_event_io.h"
#include "ttx/modifiers.h"
#include "ttx/mouse.h"
#include "ttx/mouse_event.h"
#include "ttx/mouse_event_io.h"
#include "ttx/params.h"

namespace mouse_event_io {
static void serialize() {
    using namespace ttx;

    struct Case {
        MouseEvent event;
        di::Optional<di::TransparentStringView> expected {};
        MouseEncoding encoding { MouseEncoding::SGR };
        MouseProtocol protocol { MouseProtocol::AnyEvent };
        di::Optional<MousePosition> prev_event_position {};
        MouseScrollProtocol scroll_protocol {};
        dius::tty::WindowSize window_size {};
    };

    auto cases = di::Array {
        // Protocol::None.
        Case { MouseEvent::press(MouseButton::Left, {}), {}, MouseEncoding::SGR, MouseProtocol::None },

        // Scroll protocol
        Case { MouseEvent::press(MouseButton::ScrollUp, {}),
               "\033[A"_tsv,
               MouseEncoding::SGR,
               MouseProtocol::None,
               {},
               MouseScrollProtocol { AlternateScrollMode::Enabled, ApplicationCursorKeysMode::Disabled, true } },
        Case { MouseEvent::press(MouseButton::ScrollUp, {}),
               "\033OA"_tsv,
               MouseEncoding::SGR,
               MouseProtocol::X10,
               {},
               MouseScrollProtocol { AlternateScrollMode::Enabled, ApplicationCursorKeysMode::Enabled, true } },
        Case { MouseEvent::press(MouseButton::ScrollDown, {}),
               "\033OB"_tsv,
               MouseEncoding::SGR,
               MouseProtocol::X10,
               {},
               MouseScrollProtocol { AlternateScrollMode::Enabled, ApplicationCursorKeysMode::Enabled, true } },
        Case { MouseEvent::press(MouseButton::ScrollRight, {}),
               {},
               MouseEncoding::SGR,
               MouseProtocol::None,
               {},
               MouseScrollProtocol { AlternateScrollMode::Enabled, ApplicationCursorKeysMode::Enabled, true } },
        Case { MouseEvent::press(MouseButton::ScrollUp, {}),
               {},
               MouseEncoding::SGR,
               MouseProtocol::None,
               {},
               MouseScrollProtocol { AlternateScrollMode::Enabled, ApplicationCursorKeysMode::Enabled, false } },
        Case { MouseEvent::press(MouseButton::ScrollUp, {}),
               {},
               MouseEncoding::SGR,
               MouseProtocol::None,
               {},
               MouseScrollProtocol { AlternateScrollMode::Disabled, ApplicationCursorKeysMode::Enabled, true } },

        // X10 encoding
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[M !!"_tsv, MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::Middle, MousePosition({ 94, 94 })), "\033[M!\x7f\x7f"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 94, 94 })), "\033[M\"\x7f\x7f"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 222, 222 })), "\033[M\"\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 222, 222 })), "\033[M\x60\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::ScrollDown, MousePosition({ 222, 222 })), "\033[M\x61\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::ScrollLeft, MousePosition({ 222, 222 })), "\033[M\x62\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::ScrollRight, MousePosition({ 222, 222 })), "\033[M\x63\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::_8, MousePosition({ 222, 222 })), "\033[M\xa0\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::_9, MousePosition({ 222, 222 })), "\033[M\xa1\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::_10, MousePosition({ 222, 222 })), "\033[M\xa2\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::_11, MousePosition({ 222, 222 })), "\033[M\xa3\xff\xff"_tsv,
               MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Release, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[M#\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[M\xc3\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Shift),
               "\033[M\xc7\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Alt),
               "\033[M\xcb\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Control),
               "\033[M\xd3\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }),
                          Modifiers::Alt | Modifiers::Shift | Modifiers::Control),
               "\033[M\xdf\xff\xff"_tsv, MouseEncoding::X10 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 223, 94 })), {}, MouseEncoding::X10 },

        // UTF-8 encoding
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[M !!"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::Middle, MousePosition({ 94, 94 })), "\033[M!\x7f\x7f"_tsv,
               MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 94, 94 })), "\033[M\"\x7f\x7f"_tsv,
               MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 0x10FFFF - 33, 222 })),
               "\033[M\"\xf4\x8f\xbf\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 0x10FFFF - 33, 222 })),
               "\033[M\x60\xf4\x8f\xbf\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::ScrollDown, MousePosition({ 0x10FFFF - 33, 222 })),
               "\033[M\x61\xf4\x8f\xbf\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::_11, MousePosition({ 0x10FFFF - 33, 222 })),
               "\033[M\xa3\xf4\x8f\xbf\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Release, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[M#\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[M\xc3\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Shift),
               "\033[M\xc7\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Alt),
               "\033[M\xcb\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Control),
               "\033[M\xd3\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }),
                          Modifiers::Alt | Modifiers::Shift | Modifiers::Control),
               "\033[M\xdf\xc3\xbf\xc3\xbf"_tsv, MouseEncoding::UTF8 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 0x10FFFF - 32, 94 })), {}, MouseEncoding::UTF8 },

        // SGR encoding
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::Middle, MousePosition({ 94, 94 })), "\033[<1;95;95M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 94, 94 })), "\033[<2;95;95M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 222, 222 })), "\033[<2;223;223M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 222, 222 })), "\033[<64;223;223M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::ScrollDown, MousePosition({ 222, 222 })), "\033[<65;223;223M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::ScrollLeft, MousePosition({ 222, 222 })), "\033[<66;223;223M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent::press(MouseButton::_11, MousePosition({ 222, 222 })), "\033[<131;223;223M"_tsv,
               MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Release, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[<131;223;223m"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 222, 222 })),
               "\033[<35;223;223M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[<163;223;223M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Shift),
               "\033[<167;223;223M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Alt),
               "\033[<171;223;223M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Control),
               "\033[<179;223;223M"_tsv, MouseEncoding::SGR },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }),
                          Modifiers::Alt | Modifiers::Shift | Modifiers::Control),
               "\033[<191;223;223M"_tsv, MouseEncoding::SGR },

        // UXRVT encoding
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[32;1;1M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::Middle, MousePosition({ 94, 94 })), "\033[33;95;95M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 94, 94 })), "\033[34;95;95M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 222, 222 })), "\033[34;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 222, 222 })), "\033[96;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::ScrollDown, MousePosition({ 222, 222 })), "\033[97;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::ScrollLeft, MousePosition({ 222, 222 })), "\033[98;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent::press(MouseButton::_11, MousePosition({ 222, 222 })), "\033[163;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Release, MouseButton::_11, MousePosition({ 222, 222 })),
               "\033[35;223;223M"_tsv, MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 222, 222 })), "\033[67;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 })), "\033[195;223;223M"_tsv,
               MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Shift),
               "\033[199;223;223M"_tsv, MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Alt),
               "\033[203;223;223M"_tsv, MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }), Modifiers::Control),
               "\033[211;223;223M"_tsv, MouseEncoding::URXVT },
        Case { MouseEvent(MouseEventType::Move, MouseButton::_11, MousePosition({ 222, 222 }),
                          Modifiers::Alt | Modifiers::Shift | Modifiers::Control),
               "\033[223;223;223M"_tsv, MouseEncoding::URXVT },

        // SGR pixels encoding
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 }, MouseCoordinate { 3, 2 })),
               "\033[<0;3;2M"_tsv, MouseEncoding::SGRPixels },
        Case {
            MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })),
            "\033[<0;5;8M"_tsv,
            MouseEncoding::SGRPixels,
            MouseProtocol::AnyEvent,
            {},
            {},
            { 100, 100, 1000, 1600 },
        },

        // X10 protocol
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::X10 },
        Case { MouseEvent::press(MouseButton::Middle, MousePosition({ 0, 0 })), "\033[<1;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::X10 },
        Case { MouseEvent::press(MouseButton::Right, MousePosition({ 0, 0 })), "\033[<2;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::X10 },
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 }), Modifiers::Control), "\033[<0;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::X10 },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::X10 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::ScrollUp, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::X10 },
        Case { MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::X10 },

        // VT 200 protocol
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::VT200 },
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 }), Modifiers::Control), "\033[<16;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::VT200 },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::VT200 },
        Case { MouseEvent(MouseEventType::Move, MouseButton::ScrollUp, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::VT200 },
        Case { MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1m"_tsv,
               MouseEncoding::SGR, MouseProtocol::VT200 },

        // Button event protocol
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::BtnEvent },
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 }), Modifiers::Control), "\033[<16;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::BtnEvent },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::BtnEvent },
        Case { MouseEvent(MouseEventType::Move, MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<96;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::BtnEvent },
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::BtnEvent },
        Case { MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1m"_tsv,
               MouseEncoding::SGR, MouseProtocol::BtnEvent },

        // Any event protocol
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::AnyEvent },
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 }), Modifiers::Control), "\033[<16;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::AnyEvent },
        Case { MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::AnyEvent },
        Case { MouseEvent(MouseEventType::Move, MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<96;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::AnyEvent },
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 0, 0 })), "\033[<35;1;1M"_tsv,
               MouseEncoding::SGR, MouseProtocol::AnyEvent },
        Case { MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 0, 0 })), "\033[<64;1;1m"_tsv,
               MouseEncoding::SGR, MouseProtocol::AnyEvent },

        // Skip redundant motion events
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 0, 0 })),
               {},
               MouseEncoding::SGR,
               MouseProtocol::AnyEvent,
               MousePosition({ 0, 0 }) },
        Case { MouseEvent(MouseEventType::Move, MouseButton::None, MousePosition({ 3, 3 }, MouseCoordinate { 6, 6 })),
               {},
               MouseEncoding::SGRPixels,
               MouseProtocol::AnyEvent,
               MousePosition({ 0, 0 }, MouseCoordinate { 6, 6 }) },
        Case { MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })), "\033[<0;1;1M"_tsv, MouseEncoding::SGR,
               MouseProtocol::AnyEvent, MousePosition({ 0, 0 }) },
    };

    for (auto const& test_case : cases) {
        auto result =
            serialize_mouse_event(test_case.event, test_case.protocol, test_case.encoding,
                                  test_case.prev_event_position, test_case.scroll_protocol, test_case.window_size);
        ASSERT_EQ(test_case.expected, result);
    }
}

static void parse() {
    using namespace ttx;

    struct Case {
        CSI csi {};
        di::Optional<MouseEvent> expected {};
        di::Optional<dius::tty::WindowSize> window_size_if_using_pixels {};
    };

    auto cases = di::Array {
        Case {
            CSI(""_s, { { 0 }, { 1 }, { 1 } }, U'M'),
            {},
        },
        Case {
            CSI("<"_s, { { 1000 }, { 1 }, { 1 } }, U'M'),
            {},
        },
        Case {
            CSI("<"_s, { { 0 }, { 1 }, { 1 } }, U'M'),
            MouseEvent::press(MouseButton::Left, MousePosition({ 0, 0 })),
        },
        Case {
            CSI("<"_s, { { 2 }, { 1 }, { 1 } }, U'M'),
            MouseEvent::press(MouseButton::Right, MousePosition({ 0, 0 })),
        },
        Case {
            CSI("<"_s, { { 2 }, { 0 }, { 0 } }, U'M'),
            MouseEvent::press(MouseButton::Right, MousePosition({ 0, 0 })),
        },
        Case {
            CSI("<"_s, { { 64 }, { 5 }, { 6 } }, U'M'),
            MouseEvent::press(MouseButton::ScrollUp, MousePosition({ 4, 5 })),
        },
        Case {
            CSI("<"_s, { { 96 }, { 5 }, { 6 } }, U'M'),
            MouseEvent(MouseEventType::Move, MouseButton::ScrollUp, MousePosition({ 4, 5 })),
        },
        Case {
            CSI("<"_s, { { 64 }, { 5 }, { 6 } }, U'm'),
            MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 4, 5 })),
        },
        Case {
            CSI("<"_s, { { 68 }, { 5 }, { 6 } }, U'm'),
            MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 4, 5 }), Modifiers::Shift),
        },
        Case {
            CSI("<"_s, { { 76 }, { 5 }, { 6 } }, U'm'),
            MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 4, 5 }),
                       Modifiers::Shift | Modifiers::Alt),
        },
        Case {
            CSI("<"_s, { { 92 }, { 5 }, { 6 } }, U'm'),
            MouseEvent(MouseEventType::Release, MouseButton::ScrollUp, MousePosition({ 4, 5 }),
                       Modifiers::Shift | Modifiers::Alt | Modifiers::Control),
        },
        Case {
            CSI("<"_s, { { 92 }, { 121 }, { 156 } }, U'm'),
            MouseEvent(MouseEventType::Release, MouseButton::ScrollUp,
                       MousePosition({ 12, 7 }, MouseCoordinate(121, 156)),
                       Modifiers::Shift | Modifiers::Alt | Modifiers::Control),
            dius::tty::WindowSize { 80, 100, 1000, 1600 },
        },
    };

    for (auto const& test_case : cases) {
        auto result = mouse_event_from_csi(test_case.csi, test_case.window_size_if_using_pixels);
        ASSERT_EQ(test_case.expected, result);
    }
}

TEST(key_event_io, serialize)
TEST(key_event_io, parse)
}
