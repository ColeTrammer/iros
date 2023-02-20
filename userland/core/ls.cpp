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
    auto iterator = TRY(di::create<dius::fs::RecursiveDirectoryIterator>(di::move(path)));
    for (auto directory : iterator) {
        auto const& entry = TRY(directory);
        dius::println("{}: {},{}"_sv, entry.path(), TRY(entry.is_regular_file()),
                      dius::fs::is_regular_file(TRY(entry.status())));
    }
    return {};
}
}

int main(int argc, char** argv) {
    auto args = di::Vector<di::TransparentStringView> {};
    for (int i = 0; i < argc; i++) {
        char* arg = argv[i];
        size_t len = 0;
        while (arg[len] != '\0') {
            len++;
        }
        args.push_back({ arg, len });
    }

    auto as_span = args.span();
    auto parser = di::get_cli_parser<ls::Args>();
    auto result = parser.parse(as_span);
    if (!result) {
        dius::eprintln("Failed to parse command line arguments"_sv);
        return 2;
    }

    using Result = decltype(ls::main(*result));
    if constexpr (di::concepts::Expected<Result>) {
        auto main_result = ls::main(*result);
        if (!main_result) {
            return 1;
        }
    } else {
        (void) ls::main(*result);
    }
    return 0;
}

// DIUS_MAIN(ls::Args, ls)
