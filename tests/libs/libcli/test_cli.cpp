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

    Flag a_flag = Flag::boolean(args.c).short_name('c').long_name("crash").description("Cause the program to crash");
    Flag b_flag = Flag::value<int>(args.i).short_name('i').long_name("int").description("The number of iterations");

    Argument dest_arg = Argument::single<StringView>(args.dest, "dest").description("destination to destroy");

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
