#include <dius/prelude.h>

namespace ls {
struct Args {
    di::PathView path { "."_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("ls"_sv, "List directory contents"_sv)
            .flag<&Args::path>('p', "path"_tsv, "Directory path to list from"_sv);
    }
};

di::Result<void> main(Args& args) {
    auto path = di::create<di::Path>(args.path);
    auto iterator = TRY(di::create<dius::fs::DirectoryIterator>(di::move(path)));
    for (auto directory : iterator) {
        auto const& entry = TRY(directory);
        dius::println("{}: {},{}"_sv, entry.path(), TRY(entry.is_regular_file()),
                      dius::fs::is_regular_file(TRY(entry.status())));
    }
    return {};
}
}

DIUS_MAIN(ls::Args, ls)
