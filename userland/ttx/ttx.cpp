#include "di/cli/parser.h"
#include "di/container/string/string_view.h"
#include "di/io/writer_print.h"
#include "di/sync/synchronized.h"
#include "dius/main.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/terminal.h"
#include "ttx/terminal_input.h"
#include "ttx/utf8_stream_decoder.h"

namespace ttx {
struct Args {
    di::Vector<di::TransparentStringView> command;
    bool help { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("ttx"_sv, "Terminal multiplexer"_sv)
            .argument<&Args::command>("COMMAND"_sv, "Program to run in terminal"_sv, true)
            .help();
    }
};

static auto spawn_child(Args const& args, dius::SyncFile& pty) -> di::Result<void> {
    auto tty_path = TRY(pty.get_psuedo_terminal_path());

    auto child_result = TRY(dius::system::Process(args.command | di::transform(di::to_owned) | di::to<di::Vector>())
                                .with_new_session()
                                .with_file_open(0, di::move(tty_path), dius::OpenMode::ReadWrite)
                                .with_file_dup(0, 1)
                                .with_file_dup(0, 2)
                                .with_file_close(pty.file_descriptor())
                                .spawn_and_wait());
    if (child_result.exit_code() != 0) {
        dius::eprintln("Exited with code: {}"_sv, child_result.exit_code());
    }
    return {};
}

static auto main(Args& args) -> di::Result<void> {
    auto terminal_size = TRY(dius::stdin.get_tty_window_size());
    auto pty_controller = TRY(dius::open_psuedo_terminal_controller(dius::OpenMode::ReadWrite, terminal_size));
    auto done = di::Atomic<bool> { false };
    auto process_waiter = TRY(dius::Thread::create([&] {
        auto guard = di::ScopeExit([&] {
            done.store(true, di::MemoryOrder::Release);
        });
        auto result = spawn_child(args, pty_controller);
        if (!result.has_value()) {
            return dius::eprintln("Failed to spawn process: {}"_sv, result.error().message());
        }
    }));
    auto process_waiter_guard = di::ScopeExit([&] {
        (void) process_waiter.join();
    });

    // Setup - raw mode
    auto _ = TRY(dius::stdin.enter_raw_mode());

    // Setup - alternate screen buffer.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049h\033[H\033[2J"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049l"_sv);
    });

    // Setup - disable autowrap.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?7l"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?7h"_sv);
    });

    // Setup - kitty key mode.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[>31u"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[<u"_sv);
    });

    auto log = TRY(dius::open_sync("/tmp/ttx.log"_pv, dius::OpenMode::WriteClobber));

    auto terminal = di::Synchronized<Terminal>(pty_controller);
    terminal.get_assuming_no_concurrent_accesses().set_visible_size(terminal_size.rows, terminal_size.cols);

    TRY(dius::system::mask_signal(dius::Signal::WindowChange));

    auto input_thread = TRY(dius::Thread::create([&] -> void {
        auto _ = di::ScopeExit([&] {
            done.store(true, di::MemoryOrder::Release);
            // Ensure the SIGWINCH thread exits.
            (void) dius::system::send_signal(dius::system::get_process_id(), dius::Signal::WindowChange);
        });

        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        auto exit = false;
        auto parser = TerminalInputParser {};
        auto utf8_decoder = Utf8StreamDecoder {};
        while (!exit) {
            auto nread = dius::stdin.read_some(buffer.span());
            if (!nread.has_value()) {
                break;
            }

            auto utf8_string = utf8_decoder.decode(buffer | di::take(*nread));
            auto events = parser.parse(utf8_string);
            for (auto const& event : events) {
                if (auto ev = di::get_if<KeyEvent>(event)) {
                    if (ev->key() == Key::Q && !!(ev->modifiers() & Modifiers::Control)) {
                        exit = true;
                        break;
                    }

                    auto [application_cursor_keys_mode,
                          key_reporting_flags] = terminal.with_lock([&](Terminal& terminal) {
                        return di::Tuple { terminal.application_cursor_keys_mode(), terminal.key_reporting_flags() };
                    });

                    auto serialized_event =
                        ttx::serialize_key_event(*ev, application_cursor_keys_mode, key_reporting_flags);
                    if (serialized_event) {
                        if (!pty_controller.write_exactly(di::as_bytes(serialized_event.value().span()))) {
                            exit = true;
                            break;
                        }
                    }
                }
            }
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) input_thread.join();
    });

    auto output_thread = TRY(dius::Thread::create([&] -> void {
        auto parser = EscapeSequenceParser();
        auto cursor_row = 0_u32;
        auto cursor_col = 0_u32;
        auto last_graphics_rendition = GraphicsRendition {};

        auto utf8_decoder = Utf8StreamDecoder {};
        while (!done.load(di::MemoryOrder::Acquire)) {
            auto buffer = di::Vector<byte> {};
            buffer.resize(16384);

            auto nread = pty_controller.read_some(buffer.span());
            if (!nread.has_value()) {
                break;
            }

            auto utf8_string = utf8_decoder.decode(buffer | di::take(*nread));

            auto parser_result = parser.parse_application_escape_sequences(utf8_string);
            terminal.with_lock([&](Terminal& terminal) {
                terminal.on_parser_results(parser_result.span());

                // Draw
                if (terminal.allowed_to_draw()) {
                    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2026h"_sv);
                    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?25l"_sv);
                    for (auto const& [r, row] : di::enumerate(terminal.rows())) {
                        for (auto const& [c, cell] : di::enumerate(row)) {
                            if (cell.dirty) {
                                if (cursor_row != r || cursor_col != c) {
                                    cursor_row = r;
                                    cursor_col = c;
                                    di::writer_print<di::String::Encoding>(dius::stdin, "\033[{};{}H"_sv,
                                                                           cursor_row + 1, cursor_col + 1);
                                }

                                if (cell.graphics_rendition != last_graphics_rendition) {
                                    last_graphics_rendition = cell.graphics_rendition;

                                    di::writer_print<di::String::Encoding>(dius::stdin, "\033[{}m"_sv,
                                                                           last_graphics_rendition.as_csi_params());
                                }

                                di::writer_print<di::String::Encoding>(dius::stdin, "{}"_sv, cell.ch);
                                cursor_col = di::min(cursor_col + 1, terminal_size.cols - 1);
                            }
                        }
                    }
                    if (!terminal.cursor_hidden()) {
                        cursor_row = terminal.cursor_row();
                        cursor_col = terminal.cursor_col();
                        di::writer_print<di::String::Encoding>(dius::stdin, "\033[{};{}H"_sv, cursor_row + 1,
                                                               cursor_col + 1);
                        di::writer_print<di::String::Encoding>(dius::stdin, "\033[{} q"_sv,
                                                               i32(terminal.cursor_style()));
                        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?25h"_sv);
                    }
                    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2026l"_sv);
                }
            });
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) output_thread.join();
    });

    while (!done.load(di::MemoryOrder::Acquire)) {
        if (!dius::system::wait_for_signal(dius::Signal::WindowChange)) {
            break;
        }
        if (done.load(di::MemoryOrder::Acquire)) {
            break;
        }

        auto terminal_size = dius::stdin.get_tty_window_size();
        if (!terminal_size) {
            break;
        }

        terminal.with_lock([&](Terminal& terminal) {
            (void) pty_controller.set_tty_window_size(terminal_size.value());
            terminal.set_visible_size(terminal_size.value().rows, terminal_size.value().cols);
        });
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
