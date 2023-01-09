#include <dius/prelude.h>

namespace cps {
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

    auto buffer = di::StaticVector<di::Byte, decltype(131072_zic)> {};

    while (auto nread =
               TRY_OR_ERROR_LOG(source.read({ buffer.data(), buffer.capacity() }), "Failed to read from `{}': {}"_sv, args.source)) {
        TRY_OR_ERROR_LOG(destination.write({ buffer.data(), nread }), "Failed to write to `{}': {}"_sv, args.destination);
    }

    return {};
}
}

DIUS_MAIN(cps::Args, cps)