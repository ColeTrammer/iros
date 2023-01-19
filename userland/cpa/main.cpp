#include <dius/prelude.h>

namespace cpa {
struct Args {
    di::PathView source;
    di::PathView destination;

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("Copy Synchronously"_sv, "Copy source into destination use sync IO."_sv)
            .flag<&Args::source>('s', "source"_tsv, "Source file to copy from"_sv, true)
            .flag<&Args::destination>('d', "destination"_tsv, "Destination location to copy to"_sv, true);
    }
};

#define TRY_OR_ERROR_LOG(expr, format, ...)                                      \
    TRY((expr) | di::if_error([&](auto const& error) {                           \
            dius::error_log(format, __VA_ARGS__ __VA_OPT__(, ) error.message()); \
        }))

di::Result<void> main(Args const& args) {
    auto context = TRY_OR_ERROR_LOG(di::create<dius::IoContext>(), "Failed to create execution context: {}"_sv);
    auto scheduler = context.get_scheduler();

    auto buffer = di::StaticVector<di::Byte, decltype(131072_zic)> {};

    auto source = args.source | di::to<di::Path>();
    auto destination = args.destination | di::to<di::Path>();

    auto open_source = di::execution::async_open(scheduler, di::move(source), dius::OpenMode::Readonly);
    auto open_destination = di::execution::async_open(scheduler, di::move(destination), dius::OpenMode::WriteClobber);

    auto task = di::execution::with(di::move(open_source), [&](auto& source) {
        return di::execution::with(di::move(open_destination), [&](auto& destination) {
            return di::execution::async_read_some(source, di::Span { buffer.data(), buffer.capacity() }) |
                   di::execution::let_value([&](size_t& nread) {
                       return di::execution::just_void_or_stopped(nread == 0) | di::execution::let_value([&] {
                                  return di::execution::async_write_exactly(destination, di::Span { buffer.data(), nread });
                              });
                   }) |
                   di::execution::repeat_effect | di::execution::let_stopped([] {
                       return di::execution::just();
                   });
        });
    });

    return di::sync_wait_on(context, di::move(task)) % di::into_void;
}
}

DIUS_MAIN(cpa::Args, cpa)