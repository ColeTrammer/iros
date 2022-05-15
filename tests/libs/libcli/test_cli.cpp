#include <cli/cli.h>
#include <liim/fixed_array.h>
#include <test/test.h>

struct Args {
    bool c;
    Option<int> i;
    StringView dest;
};

static Args args;

constexpr auto cli_parser = [] {
    using namespace Cli;

    auto a_flag = Flag::boolean(args.c, 'c', "crash"sv, "Cause the program to crash"sv);
    auto b_flag = Flag::value<int>(args.i, 'i', "int"sv, "The number of iterations"sv);

    auto dest_arg = Argument::with<StringView>(args.dest, "dest"sv, "destination to destroy"sv);

    return Parser { FixedArray { a_flag, b_flag }, FixedArray { dest_arg } };
}();

static void do_cli_test(Span<StringView> input, Function<void()> validate_result) {
    args.c = false;
    args.i = {};
    args.dest = StringView {};

    auto result = cli_parser.parse(input);
    if (!result) {
        auto message = format("{}", result.error());
        EXPECT_EQ(message, "Not an error");
        EXPECT(false);
    }

    validate_result();
}

TEST(cli, basic) {
    FixedArray input = { "program"sv, "-c"sv, "file"sv };
    do_cli_test(input.span(), [] {
        EXPECT_EQ(args.c, true);
        EXPECT_EQ(args.dest, "file"sv);
        EXPECT(!args.i);
    });
}
