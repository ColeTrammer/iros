#include <eventloop/event.h>
#include <liim/vector.h>
#include <tinput/terminal_input_parser.h>

namespace TInput {
static constexpr uint8_t char_to_control(uint8_t c) {
    return c & 0x1F;
}

static constexpr uint8_t control_to_char(uint8_t c) {
    if (c == char_to_control('?')) {
        return '?';
    }
    return c | 0x40;
}

static constexpr App::Key char_to_key(uint8_t c) {
    if (isalpha(c)) {
        return static_cast<App::Key>(toupper(c) + 1 - 'A');
    }
    return App::Key::None;
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

Generator<Ext::StreamResult> TerminalInputParser::handle_ss3() {
    co_return;
}

Generator<Ext::StreamResult> TerminalInputParser::handle_csi() {
    co_return;
}

Generator<Ext::StreamResult> TerminalInputParser::handle_escape() {
    auto maybe_byte = m_reader.next_byte();
    if (!maybe_byte) {
        enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Escape, 0));
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
            enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", char_to_key(*maybe_byte), App::KeyModifier::Alt));
            co_return;
    }
}

void TerminalInputParser::handle_control_byte(uint8_t byte) {
    auto character = control_to_char(byte);
    switch (character) {
        case 'w':
            // NOTE: There is no way to distingish between ctrl+Backspace and ctrl+W
            return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Backspace, App::KeyModifier::Control));
        case '?':
            return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Backspace, 0));
    }

    return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", char_to_key(character), App::KeyModifier::Control));
}

void TerminalInputParser::handle_regular_byte(uint8_t byte) {
    switch (byte) {
        case '\r':
            return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Enter, 0));
        case '\b':
        case 127:
            return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Backspace, 0));
        case '\t':
            return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, "", App::Key::Tab, 0));
    }

    if (iscntrl(byte)) {
        return handle_control_byte(byte);
    }

    return enqueue_event(make_unique<App::KeyEvent>(App::KeyEventType::Down, String(byte), App::Key::None, 0));
}

Generator<Ext::StreamResult> TerminalInputParser::handle() {
    for (;;) {
        uint8_t byte;
        co_yield next_byte(byte);

        if (byte == '\033') {
            co_yield handle_escape();
            continue;
        }

        handle_regular_byte(byte);
    }
}
}
