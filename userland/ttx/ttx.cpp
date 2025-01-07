#include "di/cli/parser.h"
#include "dius/main.h"
#include "dius/print.h"

namespace ttx {
struct Args {
    bool help { false };

    constexpr static auto get_cli_parser() { return di::cli_parser<Args>("ttx"_sv, "Terminal multiplexer"_sv).help(); }
};

static auto main(Args&) -> di::Result<void> {
    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
