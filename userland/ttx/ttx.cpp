#include "di/cli/parser.h"
#include "di/container/string/string_view.h"
#include "di/io/writer_print.h"
#include "dius/main.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"
#include "escape_sequence_parser.h"
#include "terminal.h"
#include "terminal_input.h"

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
    // 1. Create a psuedo terminal. This would involve a bunch of stuff:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/pseudo_terminal.cpp
    // 2. Read input from psuedo terminal. This updates the internal state:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/tty_parser.cpp
    // 3. Update the terminal. This uses the internal state:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/tty.cpp

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
    // di::writer_print<di::String::Encoding>(dius::stdin, "\033[>31u"_sv);
    // auto _ = di::ScopeExit([&] {
    //     di::writer_print<di::String::Encoding>(dius::stdin, "\033[<u"_sv);
    // });

    auto input_thread = TRY(dius::Thread::create([&] -> void {
        auto _ = di::ScopeExit([&] {
            done.store(true, di::MemoryOrder::Release);
        });

        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        auto exit = false;
        auto parser = TerminalInputParser {};
        while (!exit) {
            auto nread = dius::stdin.read_some(buffer.span());
            if (!nread.has_value()) {
                break;
            }

            if (!pty_controller.write_exactly(buffer | di::take(*nread))) {
                break;
            }

            // Instead of failing on UTF-8, instead we should insert potentially buffer data.
            // And also emit replacement characters.
            auto utf8_string = buffer | di::take(*nread) | di::transform([](byte b) {
                                   return c8(b);
                               }) |
                               di::to<di::String>();
            if (!utf8_string) {
                break;
            }
            auto events = parser.parse(*utf8_string);
            for (auto const& event : events) {
                if (auto ev = di::get_if<KeyEvent>(event)) {
                    if (ev->key() == Key::Q && !!(ev->modifiers() & Modifiers::Control)) {
                        exit = true;
                        break;
                    }
                }
            }
        }
        done.store(true, di::MemoryOrder::Release);
    }));
    auto input_thread_guard = di::ScopeExit([&] {
        (void) input_thread.join();
    });

    auto parser = EscapeSequenceParser();
    auto terminal = Terminal(pty_controller);
    terminal.set_visible_size(terminal_size.rows, terminal_size.cols);
    auto cursor_row = 0_u32;
    auto cursor_col = 0_u32;
    auto last_graphics_rendition = GraphicsRendition {};

    auto log = TRY(dius::open_sync("/tmp/ttx.log"_pv, dius::OpenMode::WriteClobber));
    while (!done.load(di::MemoryOrder::Acquire)) {
        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        auto nread = pty_controller.read_some(buffer.span());
        if (!nread.has_value()) {
            break;
        }

        // Instead of failing on UTF-8, instead we should insert potentially buffer data.
        // And also emit replacement characters.
        auto utf8_string = buffer | di::take(*nread) | di::transform([](byte b) {
                               return c8(b);
                           }) |
                           di::to<di::String>();
        if (!utf8_string) {
            break;
        }

        auto safe_string = *utf8_string | di::transform([](c32 code_point) -> di::String {
            if (code_point >= 32 && code_point != 127) {
                return di::single(code_point) | di::to<di::String>();
            }
            return *di::present("<{:2x}>"_sv, (i32) code_point);
        }) | di::join | di::to<di::String>();
        (void) di::writer_println<di::container::string::Utf8Encoding>(log, "\"{}\""_sv, safe_string);

        auto parser_result = parser.parse(*utf8_string);
        for (auto const& result : parser_result) {
            (void) di::writer_println<di::container::string::Utf8Encoding>(log, "{}"_sv,
                                                                           di::visit(di::to_string, result));
        }
        terminal.on_parser_results(parser_result.span());

        // Draw
        if (terminal.allowed_to_draw()) {
            di::writer_print<di::String::Encoding>(dius::stdin, "\033[?25l"_sv);
            for (auto const& [r, row] : di::enumerate(terminal.rows())) {
                for (auto const& [c, cell] : di::enumerate(row)) {
                    if (cell.dirty) {
                        if (cursor_row != r || cursor_col != c) {
                            cursor_row = r;
                            cursor_col = c;
                            di::writer_print<di::String::Encoding>(dius::stdin, "\033[{};{}H"_sv, cursor_row + 1,
                                                                   cursor_col + 1);
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
                di::writer_print<di::String::Encoding>(dius::stdin, "\033[{};{}H"_sv, cursor_row + 1, cursor_col + 1);
                di::writer_print<di::String::Encoding>(dius::stdin, "\033[?25h"_sv);
            }
        }
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
