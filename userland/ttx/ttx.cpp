#include "di/cli/parser.h"
#include "di/container/string/string_view.h"
#include "dius/main.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"

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

    auto pty_controller = TRY(dius::open_psuedo_terminal_controller(dius::OpenMode::ReadWrite, { 80, 25 }));
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

    auto input_thread = TRY(dius::Thread::create([&] {
        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        for (;;) {
            auto nread = dius::stdin.read_some(buffer.span());
            if (!nread.has_value()) {
                break;
            }

            if (!pty_controller.write_exactly(buffer | di::take(*nread))) {
                break;
            }
        }
    }));
    auto input_thread_guard = di::ScopeExit([&] {
        (void) input_thread.join();
    });

    while (!done.load(di::MemoryOrder::Acquire)) {
        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        auto nread = pty_controller.read_some(buffer.span());
        if (!nread.has_value()) {
            break;
        }
        auto as_string = TRY(di::create<di::String>(buffer | di::take(*nread) | di::transform([](byte x) {
                                                        return c8(x);
                                                    })));
        dius::print("{}"_sv, as_string);
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
