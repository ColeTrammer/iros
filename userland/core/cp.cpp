#include <di/cli/prelude.h>
#include <dius/io_context.h>
#include <dius/main.h>
#include <dius/print.h>

namespace cp {
struct Args {
    di::PathView source;
    di::PathView destination;

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("Copy Synchronously"_sv, "Copy source into destination use sync IO."_sv)
            .argument<&Args::source>("SRC"_sv, "Source file to copy from"_sv, true)
            .argument<&Args::destination>("DEST"_sv, "Destination location to copy to"_sv, true);
    }
};

#define TRY_OR_ERROR_LOG(expr, format, ...)                                     \
    TRY((expr) | di::if_error([&](auto const& error) {                          \
            dius::eprintln(format, __VA_ARGS__ __VA_OPT__(, ) error.message()); \
        }))

di::Result<void> main(Args const& args) {
    auto context = TRY_OR_ERROR_LOG(di::create<dius::IoContext>(), "Failed to create execution context: {}"_sv);
    auto scheduler = context.get_scheduler();

    auto buffer = di::StaticVector<di::Byte, decltype(131072_zic)> {};

    auto source = args.source.to_owned();
    auto destination = args.destination.to_owned();

    namespace ex = di::execution;

    auto open_source = ex::async_open(scheduler, di::move(source), dius::OpenMode::Readonly);
    auto open_destination = ex::async_open(scheduler, di::move(destination), dius::OpenMode::WriteClobber);

    auto task = ex::with(di::move(open_source), [&](auto& source) {
        return ex::with(di::move(open_destination), [&](auto& destination) {
            return ex::async_read_some(source, di::Span { buffer.data(), buffer.capacity() }) |
                   ex::let_value([&](size_t& nread) {
                       return ex::just_void_or_stopped(nread == 0) | ex::let_value([&] {
                                  return ex::async_write_exactly(destination, di::Span { buffer.data(), nread });
                              });
                   }) |
                   ex::repeat_effect | ex::let_stopped([] {
                       return ex::just();
                   });
        });
    });

    return di::sync_wait_on(context, di::move(task)) % di::into_void;
}
}

DIUS_MAIN(cp::Args, cp)
