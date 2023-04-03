#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace cli {
struct Args {
    bool enable { false };
    di::PathView input { "test.txt"_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("test"_sv, "Long Description"_sv)
            .flag<&Args::enable>('e', "enable"_tsv)
            .flag<&Args::input>('i', "input"_tsv);
    }
};

constexpr void basic() {
    auto parser = di::get_cli_parser<Args>();

    {
        auto args = di::Array { "test"_tsv, "-e"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "test.txt"_pv);
    }

    {
        auto args = di::Array { "test"_tsv, "-e"_tsv, "-iinput.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }

    {
        auto args = di::Array { "test"_tsv, "-ei"_tsv, "input.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }

    {
        auto args = di::Array { "test"_tsv, "-e"_tsv, "-i"_tsv, "input.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }

    {
        auto args = di::Array { "test"_tsv, "--enable"_tsv, "--input"_tsv, "input.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }

    {
        auto args = di::Array { "test"_tsv, "--enable"_tsv, "--input=input.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }
}

TESTC(cli, basic);
}
