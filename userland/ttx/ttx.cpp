#include "di/cli/parser.h"
#include "dius/main.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"

namespace ttx {
struct Args {
    bool help { false };

    constexpr static auto get_cli_parser() { return di::cli_parser<Args>("ttx"_sv, "Terminal multiplexer"_sv).help(); }
};

static auto main(Args&) -> di::Result<void> {
    // 1. Create a psuedo terminal. This would involve a bunch of stuff:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/pseudo_terminal.cpp
    // 2. Read input from psuedo terminal. This updates the internal state:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/tty_parser.cpp
    // 3. Update the terminal. This uses the internal state:
    //    https://github.com/ColeTrammer/iros/blob/legacy/libs/libterminal/tty.cpp

    // 1.
    auto pty = TRY(dius::open_psuedo_terminal_controller(dius::OpenMode::ReadWrite, { 80, 25 }));
    auto tty_path = TRY(pty.get_psuedo_terminal_path());

    auto child_result = TRY(dius::system::Process(di::Array { "ls"_ts } | di::as_rvalue | di::to<di::Vector>())
                                .with_new_session()
                                .with_file_open(0, di::move(tty_path), dius::OpenMode::ReadWrite)
                                .with_file_dup(0, 1)
                                .with_file_dup(0, 2)
                                .with_file_close(pty.file_descriptor())
                                .spawn_and_wait());
    if (child_result.exit_code() != 0) {
        dius::eprintln("Exited with code: {}"_sv, child_result.exit_code());
    }

    auto buffer = di::Vector<byte> {};
    buffer.resize(4096);

    auto nread = TRY(pty.read_some(buffer.span()));
    auto as_string = TRY(di::create<di::String>(buffer | di::take(nread) | di::transform([](byte x) {
                                                    return c8(x);
                                                })));
    dius::print("{}"_sv, as_string);

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
