#include <unistd.h>

#include "di/cli/parser.h"
#include "di/container/string/string_view.h"
#include "di/io/writer_print.h"
#include "di/sync/synchronized.h"
#include "dius/main.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"
#include "pane.h"
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

static auto main(Args& args) -> di::Result<void> {
    auto terminal_size = di::Synchronized(TRY(dius::stdin.get_tty_window_size()));
    auto pane = TRY(Pane::create(di::move(args).command, terminal_size.get_const_assuming_no_concurrent_mutations()));

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

    // Setup - capture all mouse events and use SGR mosue reporting.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1003h\033[?1006h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1006l\033[?1003l"_sv);
    });

    // Setup - enable focus events.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1004h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1004l"_sv);
    });

    // Setup - bracketed paste.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2004h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2004l"_sv);
    });

    [[maybe_unused]] auto& log = dius::stderr = TRY(dius::open_sync("/tmp/ttx.log"_pv, dius::OpenMode::WriteClobber));

    TRY(dius::system::mask_signal(dius::Signal::WindowChange));

    auto done = di::Atomic<bool>(false);
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
                    pane->event(*ev);
                } else if (auto ev = di::get_if<MouseEvent>(event)) {
                    pane->event(*ev);
                } else if (auto ev = di::get_if<FocusEvent>(event)) {
                    pane->event(*ev);
                } else if (auto ev = di::get_if<PasteEvent>(event)) {
                    pane->event(*ev);
                }
            }
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) input_thread.join();
    });

    auto render_thread = TRY(dius::Thread::create([&] -> void {
        auto renderer = Renderer();

        while (!done.load(di::MemoryOrder::Acquire)) {
            auto size = terminal_size.with_lock([&](dius::tty::WindowSize& terminal_size) {
                return terminal_size;
            });

            renderer.start(size);
            auto cursor = pane->draw(renderer);
            (void) renderer.finish(dius::stdin, cursor);

            // TODO: use a sleep method in dius. And sleep until a deadline to prevent drift.
            usleep(16000);
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) render_thread.join();
    });

    while (!done.load(di::MemoryOrder::Acquire)) {
        if (!dius::system::wait_for_signal(dius::Signal::WindowChange)) {
            break;
        }
        if (done.load(di::MemoryOrder::Acquire)) {
            break;
        }

        terminal_size.with_lock([&](dius::tty::WindowSize& terminal_size) {
            auto size = dius::stdin.get_tty_window_size();
            if (!size) {
                return;
            }
            terminal_size = size.value();
            pane->resize(terminal_size);
        });
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
