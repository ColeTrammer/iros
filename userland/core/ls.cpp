#include <dius/filesystem/prelude.h>
#include <dius/main.h>

namespace ls {
struct Args {
    di::PathView path { "."_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("ls"_sv, "List directory contents"_sv)
            .argument<&Args::path>("PATH"_sv, "Directory path to list from"_sv, false);
    }
};

di::Result<void> main([[maybe_unused]] Args& args) {
    auto path = di::create<di::Path>(args.path);
    auto iterator =
        TRY(di::create<dius::fs::RecursiveDirectoryIterator>(di::move(path)) | di::if_error([](auto&& error) {
                dius::println("ls: {}"_sv, error.message());
            }));
    for (auto directory : iterator) {
        auto const& entry = TRY(directory);
        dius::println("{}: {},{}"_sv, entry.path(), TRY(entry.is_regular_file()),
                      dius::fs::is_regular_file(TRY(entry.status())));
    }
    return {};
}
}

DIUS_MAIN(ls::Args, ls)
