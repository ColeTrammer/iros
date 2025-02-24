#include "ttx/pane.h"

#include "di/container/string/string_view.h"
#include "di/container/vector/vector.h"
#include "di/function/container/function.h"
#include "di/sync/atomic.h"
#include "di/sync/memory_order.h"
#include "di/vocab/pointer/box.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/tty.h"
#include "ttx/paste_event.h"
#include "ttx/renderer.h"
#include "ttx/terminal.h"
#include "ttx/utf8_stream_decoder.h"

namespace ttx {
static auto spawn_child(di::Vector<di::TransparentStringView> command, dius::SyncFile& pty)
    -> di::Result<dius::system::ProcessHandle> {
    auto tty_path = TRY(pty.get_psuedo_terminal_path());

    return dius::system::Process(command | di::transform(di::to_owned) | di::to<di::Vector>())
        .with_new_session()
        .with_file_open(0, di::move(tty_path), dius::OpenMode::ReadWrite)
        .with_file_dup(0, 1)
        .with_file_dup(0, 2)
        .with_file_close(pty.file_descriptor())
        .spawn();
}

auto Pane::create(di::Vector<di::TransparentStringView> command, dius::tty::WindowSize size)
    -> di::Result<di::Box<Pane>> {
    auto pty_controller = TRY(dius::open_psuedo_terminal_controller(dius::OpenMode::ReadWrite, size));
    auto process = TRY(spawn_child(di::move(command), pty_controller));
    auto pane = di::make_box<Pane>(di::move(pty_controller), process);
    pane->m_terminal.get_assuming_no_concurrent_accesses().set_visible_size(size);

    pane->m_process_thread = TRY(dius::Thread::create([&pane = *pane] mutable {
        auto guard = di::ScopeExit([&] {
            pane.m_done.store(true, di::MemoryOrder::Release);

            if (pane.did_exit) {
                pane.did_exit();
            }
        });

        (void) pane.m_process.wait();
    }));

    pane->m_reader_thread = TRY(dius::Thread::create([&pane = *pane] -> void {
        auto parser = EscapeSequenceParser();

        auto utf8_decoder = Utf8StreamDecoder {};
        while (!pane.m_done.load(di::MemoryOrder::Acquire)) {
            auto buffer = di::Vector<byte> {};
            buffer.resize(16384);

            auto nread = pane.m_pty_controller.read_some(buffer.span());
            if (!nread.has_value()) {
                break;
            }

            auto utf8_string = utf8_decoder.decode(buffer | di::take(*nread));

            auto parser_result = parser.parse_application_escape_sequences(utf8_string);
            pane.m_terminal.with_lock([&](Terminal& terminal) {
                terminal.on_parser_results(parser_result.span());
            });
        }
    }));

    return pane;
}

auto Pane::create_mock() -> di::Box<Pane> {
    auto fake_psuedo_terminal = dius::SyncFile();
    return di::make_box<Pane>(di::move(fake_psuedo_terminal), dius::system::ProcessHandle());
}

Pane::~Pane() {
    (void) m_process.signal(dius::Signal::Hangup);
    (void) m_reader_thread.join();
    (void) m_process_thread.join();
}

auto Pane::draw(Renderer& renderer) -> RenderedCursor {
    return m_terminal.with_lock([&](Terminal& terminal) {
        if (terminal.allowed_to_draw()) {
            for (auto const& [r, row] : di::enumerate(terminal.rows())) {
                for (auto const& [c, cell] : di::enumerate(row)) {
                    if (cell.dirty) {
                        renderer.put_text(cell.ch, r, c, cell.graphics_rendition);
                    }
                }
            }
        }
        return RenderedCursor {
            .cursor_row = terminal.cursor_row(),
            .cursor_col = terminal.cursor_col(),
            .style = terminal.cursor_style(),
            .hidden = terminal.cursor_hidden(),
        };
    });
}

auto Pane::event(KeyEvent const& event) -> bool {
    auto [application_cursor_keys_mode, key_reporting_flags] = m_terminal.with_lock([&](Terminal& terminal) {
        return di::Tuple { terminal.application_cursor_keys_mode(), terminal.key_reporting_flags() };
    });

    auto serialized_event = ttx::serialize_key_event(event, application_cursor_keys_mode, key_reporting_flags);
    if (serialized_event) {
        (void) m_pty_controller.write_exactly(di::as_bytes(serialized_event.value().span()));
        return true;
    }
    return false;
}

auto Pane::event(MouseEvent const& event) -> bool {
    auto [application_cursor_keys_mode, alternate_scroll_mode, mouse_protocol, mouse_encoding,
          in_alternate_screen_buffer, window_size] = m_terminal.with_lock([&](Terminal& terminal) {
        return di::Tuple {
            terminal.application_cursor_keys_mode(),
            terminal.alternate_scroll_mode(),
            terminal.mouse_protocol(),
            terminal.mouse_encoding(),
            terminal.in_alternate_screen_buffer(),
            terminal.size(),
        };
    });

    auto serialized_event = serialize_mouse_event(
        event, mouse_protocol, mouse_encoding, m_last_mouse_position,
        { alternate_scroll_mode, application_cursor_keys_mode, in_alternate_screen_buffer }, window_size);
    if (serialized_event.has_value()) {
        (void) m_pty_controller.write_exactly(di::as_bytes(serialized_event.value().span()));
        return true;
    }
    m_last_mouse_position = event.position();

    // Support mouse scrolling.
    if (event.button() == MouseButton::ScrollUp && event.type() == MouseEventType::Press) {
        m_terminal.with_lock([&](Terminal& terminal) {
            terminal.scroll_up();
        });
        return true;
    }
    if (event.button() == MouseButton::ScrollDown && event.type() == MouseEventType::Press) {
        m_terminal.with_lock([&](Terminal& terminal) {
            terminal.scroll_down();
        });
        return true;
    }
    return false;
}

auto Pane::event(FocusEvent const& event) -> bool {
    auto [focus_event_mode] = m_terminal.with_lock([&](Terminal& terminal) {
        return di::Tuple { terminal.focus_event_mode() };
    });

    auto serialized_event = serialize_focus_event(event, focus_event_mode);
    if (serialized_event) {
        (void) m_pty_controller.write_exactly(di::as_bytes(serialized_event.value().span()));
        return true;
    }
    return false;
}

auto Pane::event(PasteEvent const& event) -> bool {
    auto [bracketed_paste_mode] = m_terminal.with_lock([&](Terminal& terminal) {
        return di::Tuple { terminal.bracked_paste_mode() };
    });

    auto serialized_event = serialize_paste_event(event, bracketed_paste_mode);
    (void) m_pty_controller.write_exactly(di::as_bytes(serialized_event.span()));
    return true;
}

void Pane::resize(dius::tty::WindowSize const& size) {
    m_terminal.with_lock([&](Terminal& terminal) {
        terminal.set_visible_size(size);
    });
}

void Pane::exit() {
    (void) m_process.signal(dius::Signal::Hangup);
}
}
