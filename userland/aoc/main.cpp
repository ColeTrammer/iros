#include <di/prelude.h>
#include <dius/prelude.h>

#include "aoc_problem_registry.h"

namespace aoc {
struct Args {
    di::Optional<di::PathView> input;
    int year { 2022 };
    int day;
    bool part_b { false };
    bool test { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("aoc"_sv, "Advent of Code Solver"_sv)
            .flag<&Args::input>('i', "input"_tsv, "Input file path"_sv)
            .flag<&Args::year>('y', "year"_tsv, "Year to solve"_sv)
            .flag<&Args::day>('d', "day"_tsv, "Day to solve"_sv, true)
            .flag<&Args::part_b>('b', "part-b"_tsv, "Run part b"_sv)
            .flag<&Args::test>('t', "test"_tsv, "Run with test input"_sv);
    }
};

di::Result<void> main(Args& args) {
    auto default_path = args.test ? "test.txt"_pv : "input.txt"_pv;
    auto path = args.input.value_or(default_path);
    auto string = TRY(dius::read_to_string(path) | di::if_error([&](auto&& error) {
                          dius::error_log("Failed to read input file '{}': {}"_sv, path.data(), error.message());
                      }));

    auto solver = AocProblemRegistry::the().lookup({ args.year, args.day, args.part_b });
    if (!solver) {
        dius::error_log("No solver found for {} day {} part {}"_sv, args.year, args.day, args.part_b ? "b"_sv : "a"_sv);
        return di::Unexpected(di::BasicError::Invalid);
    }

    (*solver)(string);
    return {};
}
}

DIUS_MAIN(aoc::Args, aoc)