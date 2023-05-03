#include <di/cli/prelude.h>
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

struct Args2 {
    bool enable { false };
    di::PathView input { "test.txt"_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args2>("test"_sv, "Long Description"_sv)
            .flag<&Args2::enable>('e', "enable"_tsv)
            .argument<&Args2::input>("FILE"_sv, "Input file"_sv);
    }
};

constexpr void arguments() {
    auto parser = di::get_cli_parser<Args2>();

    {
        auto args = di::Array { "test"_tsv, "-e"_tsv, "input.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.input, "input.txt"_pv);
    }
}

struct Args3 {
    bool enable { false };
    di::Vector<di::PathView> inputs;

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args3>("test"_sv, "Long Description"_sv)
            .flag<&Args3::enable>('e', "enable"_tsv)
            .argument<&Args3::inputs>("FILES"_sv, "Input files"_sv);
    }
};

constexpr void variadic_arguments() {
    auto parser = di::get_cli_parser<Args3>();

    {
        auto args = di::Array { "test"_tsv, "-e"_tsv, "input1.txt"_tsv, "input2.txt"_tsv };
        auto result = *parser.parse(args);
        ASSERT(result.enable);
        ASSERT_EQ(result.inputs.size(), 2);
        ASSERT_EQ(result.inputs[0], "input1.txt"_pv);
        ASSERT_EQ(result.inputs[1], "input2.txt"_pv);
    }
}

TESTC(cli, basic);
TESTC(cli, arguments)
TESTC(cli, variadic_arguments)
}
