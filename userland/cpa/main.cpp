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
    auto source = TRY_OR_ERROR_LOG(dius::open_sync(args.source, dius::OpenMode::Readonly), "Failed to open file `{}' for reading: {}"_sv,
                                   args.source);
    auto destination = TRY_OR_ERROR_LOG(dius::open_sync(args.destination, dius::OpenMode::WriteClobber),
                                        "Failed to open file `{}' for writing: {}"_sv, args.destination);

    auto context = dius::IoContext {};
    auto scheduler = context.get_scheduler();

    auto buffer = di::StaticVector<di::Byte, decltype(131072_zic)> {};

    bool should_stop = false;
    auto task = di::execution::async_read(scheduler, source.file_descriptor(), di::Span { buffer.data(), buffer.capacity() }) |
                di::execution::let_value([&](size_t nread) {
                    if (nread == 0) {
                        should_stop = true;
                    }
                    return di::execution::async_write(scheduler, destination.file_descriptor(), di::Span { buffer.data(), nread }) |
                           di::execution::then(di::into_void);
                }) |
                di::execution::repeat_effect_until([&] {
                    return should_stop;
                });

    // FIXME: this should call a sync_wait_with_context function.
    return di::sync_wait(di::move(task)) % di::into_void;
}
}

DIUS_MAIN(cpa::Args, cpa)