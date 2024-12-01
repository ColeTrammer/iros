#include <di/cli/prelude.h>
#include <dius/test/prelude.h>

namespace cli {
struct Args {
    bool enable { false };
    di::PathView input { "test.txt"_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("test"_sv, "Long Description"_sv)
            .option<&Args::enable>('e', "enable"_tsv, "D1"_sv)
            .option<&Args::input>('i', "input"_tsv, "D2"_sv);
    }
};

constexpr static void basic() {
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
            .option<&Args2::enable>('e', "enable"_tsv, "D1"_sv)
            .argument<&Args2::input>("FILE"_sv, "Input file"_sv);
    }
};

constexpr static void arguments() {
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
            .option<&Args3::enable>('e', "enable"_tsv, "D1"_sv)
            .argument<&Args3::inputs>("FILES"_sv, "Input files"_sv);
    }
};

constexpr static void variadic_arguments() {
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

constexpr static void help() {
    auto h1 = di::get_cli_parser<Args>().help_string();
    ASSERT_EQ(h1, R"~(NAME:
  test: Long Description

USAGE:
  test [OPTIONS]

OPTIONS:
  -e, --enable: D1
  -i, --input <VALUE>: D2
)~"_sv);

    auto h2 = di::get_cli_parser<Args2>().help_string();
    ASSERT_EQ(h2, R"~(NAME:
  test: Long Description

USAGE:
  test [OPTIONS] [FILE]

ARGUMENTS:
  [FILE]: Input file

OPTIONS:
  -e, --enable: D1
)~"_sv);

    auto h3 = di::get_cli_parser<Args3>().help_string();
    ASSERT_EQ(h3, R"~(NAME:
  test: Long Description

USAGE:
  test [OPTIONS] [FILES...]

ARGUMENTS:
  [FILES...]: Input files

OPTIONS:
  -e, --enable: D1
)~"_sv);
}

TESTC_GCC_NOSAN(cli, basic);
TESTC_GCC_NOSAN(cli, arguments)
TESTC_GCC_NOSAN(cli, variadic_arguments)
TESTC_GCC_NOSAN(cli, help)
}
