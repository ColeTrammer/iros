#include <dius/main.h>
#include <dius/print.h>
#include <dius/system/process.h>

namespace sh {
struct Args {
    constexpr static auto get_cli_parser() { return di::cli_parser<Args>("sh"_sv, "Iros shell"_sv); }
};

di::Result<void> main(Args&) {
    auto buffer = di::Array<di::Byte, 1> {};

    dius::println("//////////////////////////////////"_sv);
    dius::println("//  Welcome to the Iros shell!  //"_sv);
    dius::println("//////////////////////////////////"_sv);
    dius::print("$ "_sv);

    auto command = di::TransparentString {};
    while (auto result = dius::stdin.read_exactly(buffer.span())) {
        if (auto ch = buffer[0]; ch != '\n'_b) {
            command.push_back(di::to_integer<char>(ch));
            continue;
        }

        auto owned_args = command | di::split(' ') | di::transform(di::to_owned) | di::to<di::Vector>();

        auto spawn_result = dius::system::Process { di::move(owned_args) }.spawn_and_wait();
        if (!spawn_result) {
            dius::println("Failed to spawn process: {}"_sv, spawn_result.error().message());
        }

        command.clear();

        dius::print("$ "_sv);
    }
    return {};
}
}

DIUS_MAIN(sh::Args, sh)
