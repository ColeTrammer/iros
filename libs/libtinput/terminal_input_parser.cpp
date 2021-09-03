#include <eventloop/event.h>
#include <eventloop/widget_events.h>
#include <liim/vector.h>
#include <tinput/terminal_input_parser.h>

// #define TERMINAL_INPUT_PARSER_DEBUG

namespace TInput {
static constexpr uint8_t char_to_control(uint8_t c) {
    return c & 0x3F;
}

static constexpr uint8_t control_to_char(uint8_t c) {
    return c | 0x40;
}

static constexpr App::Key char_to_key(uint8_t c) {
    if (c == '\\') {
        return App::Key::Backslash;
    }
    if (isalpha(c)) {
        return static_cast<App::Key>(toupper(c) + 1 - 'A');
    }
    return App::Key::None;
}

static constexpr int convert_modifiers(int terminal_modifiers) {
    auto terminal_flags = terminal_modifiers - 1;
    int out_modifiers = 0;
    if (terminal_flags & 1) {
        out_modifiers |= App::KeyModifier::Shift;
    }
    if (terminal_flags & 2) {
        out_modifiers |= App::KeyModifier::Alt;
    }
    if (terminal_flags & 4) {
        out_modifiers |= App::KeyModifier::Control;
    }
    if (terminal_flags & 8) {
        out_modifiers |= App::KeyModifier::Meta;
    }
    return out_modifiers;
}

TerminalInputParser::TerminalInputParser() : m_handler(handle()) {}

TerminalInputParser::~TerminalInputParser() {}

Vector<UniquePtr<App::Event>> TerminalInputParser::take_events() {
    return move(m_event_buffer);
}

void TerminalInputParser::enqueue_event(UniquePtr<App::Event> event) {
    m_event_buffer.add(move(event));
}

void TerminalInputParser::stream_data(Span<const uint8_t> data) {
    m_reader.set_data(data);
    m_handler();
}

Generator<Ext::StreamResult> TerminalInputParser::next_byte(uint8_t& byte) {
    for (;;) {
        auto maybe_byte = m_reader.next_byte();
        if (!maybe_byte) {
            co_yield Ext::StreamResult::NeedsMoreInput;
            continue;
        }
        byte = *maybe_byte;
        co_return;
    }
}

void TerminalInputParser::finish_ss3(const String& escape) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
    fprintf(stderr, "SS3 escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */

    char c = '\0';
    int modifiers = 1;
    if (sscanf(escape.string(), "O%d%c", &modifiers, &c) != 2 && sscanf(escape.string(), "O%c", &c) != 1) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
        fprintf(stderr, "Bad SS3 escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */
        return;
    }

    auto key = [&] {
        switch (c) {
            case 'A':
                return App::Key::UpArrow;
            case 'B':
                return App::Key::DownArrow;
            case 'C':
                return App::Key::RightArrow;
            case 'D':
                return App::Key::LeftArrow;
            case 'F':
                return App::Key::End;
            case 'H':
                return App::Key::Home;
            case 'P':
                return App::Key::F1;
            case 'Q':
                return App::Key::F2;
            case 'R':
                return App::Key::F3;
            case 'S':
                return App::Key::F4;
            default:
                return App::Key::None;
        }
    }();

    enqueue_event(make_unique<App::KeyDownEvent>(key, convert_modifiers(modifiers), false));
}

void TerminalInputParser::finish_xterm_escape(const String& escape) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
    fprintf(stderr, "XTerm escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */

    char c = '\0';
    int modifiers = 1;
    if (sscanf(escape.string(), "[1;%d%c", &modifiers, &c) != 2 && sscanf(escape.string(), "[%c", &c) != 1) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
        fprintf(stderr, "Bad XTerm escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */
        return;
    }

    auto key = [&] {
        switch (c) {
            case 'A':
                return App::Key::UpArrow;
            case 'B':
                return App::Key::DownArrow;
            case 'C':
                return App::Key::RightArrow;
            case 'D':
                return App::Key::LeftArrow;
            case 'E':
                return App::Key::Numpad_5;
            case 'F':
                return App::Key::End;
            case 'H':
                return App::Key::Home;
            case 'P':
                return App::Key::F1;
            case 'Q':
                return App::Key::F2;
            case 'R':
                return App::Key::F3;
            case 'S':
                return App::Key::F4;
            case 'Z':
                // Special case - Shift Tab
                modifiers += 1;
                return App::Key::Tab;
            default:
                return App::Key::None;
        }
    }();

    enqueue_event(make_unique<App::KeyDownEvent>(key, convert_modifiers(modifiers), false));
}

void TerminalInputParser::finish_vt_escape(const String& escape) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
    fprintf(stderr, "VT escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */

    int key_code;
    int modifiers = 1;
    if (sscanf(escape.string(), "[%d;%d~", &key_code, &modifiers) != 2 && sscanf(escape.string(), "[%d~", &key_code) != 1) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
        fprintf(stderr, "Bad VT escape: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */
        return;
    }

    auto key = [&] {
        switch (key_code) {
            case 1:
                return App::Key::Home;
            case 2:
                return App::Key::Insert;
            case 3:
                return App::Key::Delete;
            case 4:
                return App::Key::End;
            case 5:
                return App::Key::PageUp;
            case 6:
                return App::Key::PageDown;
            case 7:
                return App::Key::Home;
            case 8:
                return App::Key::End;
            case 10:
                return App::Key::F1;
            case 12:
                return App::Key::F2;
            case 13:
                return App::Key::F3;
            case 14:
                return App::Key::F4;
            case 15:
                return App::Key::F5;
            case 17:
                return App::Key::F6;
            case 18:
                return App::Key::F7;
            case 19:
                return App::Key::F8;
            case 20:
                return App::Key::F9;
            case 21:
                return App::Key::F10;
            case 23:
                return App::Key::F11;
            case 24:
                return App::Key::F12;
            case 25:
                return App::Key::F13;
            case 26:
                return App::Key::F14;
            case 28:
                return App::Key::F15;
            case 29:
                return App::Key::F16;
            case 31:
                return App::Key::F17;
            case 32:
                return App::Key::F18;
            case 33:
                return App::Key::F19;
            case 34:
                return App::Key::F20;
            case 200:
                m_in_bracketed_paste = true;
                return App::Key::None;
            default:
                return App::Key::None;
        }
    }();

    if (key != App::Key::None) {
        enqueue_event(make_unique<App::KeyDownEvent>(key, convert_modifiers(modifiers), false));
    }
}

void TerminalInputParser::finish_sgr_mouse_event(const String& escape) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
    fprintf(stderr, "SGR mouse event: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */

    // SGR encoded mouse events (enabled with DECSET 1006)
    // Information from https://github.com/chromium/hterm/blob/master/doc/ControlSequences.md#sgr
    int cb;
    int cx;
    int cy;
    char cm;
    if (sscanf(escape.string(), "[<%d;%d;%d%c", &cb, &cx, &cy, &cm) != 4) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
        fprintf(stderr, "Bad SGR mouse event: '%s'\n", escape.string());
#endif /* TERMINAL_INPUT_PARSER_DEBUG */
        return;
    }

    bool mouse_down = escape[escape.size() - 1] == 'M';
    int z = 0;

    auto button = cb & ~0b11100;
    auto mouse_modifiers = (cb & 0b11100) >> 2;

    // NOTE: SGR does not report the Alt modifier, because, X11 reserved it
    //       for window control functionality. As such, xterm never bothered
    //       to report its status...
    auto modifiers = 0;
    if (mouse_modifiers & 1) {
        modifiers |= App::KeyModifier::Shift;
    }
    if (mouse_modifiers & 2) {
        modifiers |= App::KeyModifier::Meta;
    }
    if (mouse_modifiers & 4) {
        modifiers |= App::KeyModifier::Control;
    }

    int buttons_down = m_tracker.prev_buttons();
    switch (button) {
        case 0:
            // Left mouse button
            if (mouse_down) {
                buttons_down |= App::MouseButton::Left;
            } else {
                buttons_down &= ~App::MouseButton::Left;
            }
            break;
        case 1:
            // Middle mouse button
            if (mouse_down) {
                buttons_down |= App::MouseButton::Middle;
            } else {
                buttons_down &= ~App::MouseButton::Middle;
            }
            break;
        case 2:
            // Right mouse button
            if (mouse_down) {
                buttons_down |= App::MouseButton::Right;
            } else {
                buttons_down &= ~App::MouseButton::Right;
            }
            break;
        case 32:
        case 33:
        case 34:
            // Mouse move.
            break;
        case 64:
            // Scroll up
            z = -1;
            break;
        case 65:
            // Scroll down
            z = 1;
            break;
    }

    auto events = m_tracker.notify_mouse_event(buttons_down, cx - 1, cy - 1, z, modifiers);
    for (auto& event : events) {
        enqueue_event(move(event));
    }
}

void TerminalInputParser::finish_csi(const String& csi) {
    if (csi[csi.size() - 1] == '~') {
        return finish_vt_escape(csi);
    }
    if (csi[1] == '<') {
        return finish_sgr_mouse_event(csi);
    }
    return finish_xterm_escape(csi);
}

Generator<Ext::StreamResult> TerminalInputParser::handle_ss3() {
    auto maybe_byte = m_reader.next_byte();
    if (!maybe_byte) {
        enqueue_event(make_unique<App::KeyDownEvent>(App::Key::O, App::KeyModifier::Alt, false));
        co_return;
    }

    auto buffer = String { 'O' };
    buffer += String(*maybe_byte);

    for (;;) {
        auto last = buffer[buffer.size() - 1];
        if (!isdigit(last)) {
            finish_ss3(buffer);
            co_return;
        }

        uint8_t byte;
        co_yield next_byte(byte);

        buffer += String(byte);
    }
}

Generator<Ext::StreamResult> TerminalInputParser::handle_csi() {
    auto maybe_byte = m_reader.next_byte();
    if (!maybe_byte) {
        enqueue_event(make_unique<App::KeyDownEvent>(App::Key::RightBracket, App::KeyModifier::Alt, false));
        co_return;
    }

    auto buffer = String { '[' };
    buffer += String(*maybe_byte);

    for (;;) {
        auto last = buffer[buffer.size() - 1];
        if (!isdigit(last) && last != ';' && last != '<') {
            finish_csi(buffer);
            co_return;
        }

        uint8_t byte;
        co_yield next_byte(byte);

        buffer += String(byte);
    }
}

Generator<Ext::StreamResult> TerminalInputParser::handle_escape() {
    auto maybe_byte = m_reader.next_byte();
    if (!maybe_byte) {
        enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Escape, 0, false));
        co_return;
    }

    switch (*maybe_byte) {
        case 'O':
            co_yield handle_ss3();
            co_return;
        case '[':
            co_yield handle_csi();
            co_return;
        default:
            enqueue_event(make_unique<App::KeyDownEvent>(char_to_key(*maybe_byte), App::KeyModifier::Alt, false));
            co_return;
    }
}

void TerminalInputParser::handle_control_byte(uint8_t byte) {
    auto character = control_to_char(byte);
    switch (character) {
        case 'W':
            // NOTE: There is no way to distingish between ctrl+Backspace and ctrl+W
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Backspace, App::KeyModifier::Control, false));
        case '_':
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Backspace, 0, false));
    }

    return enqueue_event(make_unique<App::KeyDownEvent>(char_to_key(character), App::KeyModifier::Control, false));
}

void TerminalInputParser::handle_regular_byte(uint8_t byte) {
#ifdef TERMINAL_INPUT_PARSER_DEBUG
    fprintf(stderr, "Regular byte '%#.2X'\n", byte);
#endif /* TERMINAL_INPUT_PARSER_DEBUG */

    switch (byte) {
        case '\0':
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Space, App::KeyModifier::Control, false));
        case '\r':
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Enter, 0, false));
        case '\b':
        case 127:
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Backspace, 0, false));
        case '\t':
            return enqueue_event(make_unique<App::KeyDownEvent>(App::Key::Tab, 0, false));
    }

    if (iscntrl(byte)) {
        return handle_control_byte(byte);
    }

    return enqueue_event(make_unique<App::TextEvent>(String { static_cast<char>(byte) }));
}

Generator<Ext::StreamResult> TerminalInputParser::handle() {
    for (;;) {
        uint8_t byte;
        co_yield next_byte(byte);

        if (m_in_bracketed_paste) {
            m_bracketed_paste_buffer += String(byte);

            if (m_bracketed_paste_buffer.view().ends_with("\033[201~")) {
                m_in_bracketed_paste = false;
                enqueue_event(make_unique<App::TextEvent>(m_bracketed_paste_buffer.substring(0, m_bracketed_paste_buffer.size() - 6)));
                m_bracketed_paste_buffer.clear();
            }
            continue;
        }

        if (byte == '\033') {
            co_yield handle_escape();
            continue;
        }

        handle_regular_byte(byte);
    }
}
}
