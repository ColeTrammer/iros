#pragma once

#include <eventloop/forward.h>
#include <eventloop/input_tracker.h>
#include <ext/stream.h>
#include <liim/byte_io.h>
#include <liim/forward.h>
#include <liim/generator.h>
#include <liim/vector.h>

namespace TInput {
class TerminalInputParser {
public:
    TerminalInputParser();
    ~TerminalInputParser();

    Vector<UniquePtr<App::Event>> take_events();

    void stream_data(Span<const uint8_t> bytes);

private:
    Generator<Ext::StreamResult> handle();

    Generator<Ext::StreamResult> next_byte(uint8_t& byte);

    Generator<Ext::StreamResult> handle_ss3();
    Generator<Ext::StreamResult> handle_csi();
    Generator<Ext::StreamResult> handle_escape();
    void handle_control_byte(uint8_t byte);
    void handle_regular_byte(uint8_t byte);

    void enqueue_event(UniquePtr<App::Event> event);

    Generator<Ext::StreamResult> m_handler;
    Vector<UniquePtr<App::Event>> m_event_buffer;
    ByteReader m_reader;
    App::InputTracker m_tracker;
};
}
