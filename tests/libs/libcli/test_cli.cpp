#include <cli/cli.h>
#include <liim/fixed_array.h>
#include <test/test.h>

struct Args {
    bool c;
    Option<int> i;
    StringView dest;
};

constexpr auto cli_parser = [] {
    using namespace Cli;

    Flag a_flag = Flag::boolean<&Args::c>().short_name('c').long_name("crash").description("Cause the program to crash");
    Flag b_flag = Flag::value<&Args::i>().short_name('i').long_name("int").description("The number of iterations");

    Argument dest_arg = Argument::single<&Args::dest>("dest").description("destination to destroy");

    return make_parser<Args>(FixedArray { a_flag, b_flag }, FixedArray { dest_arg });
}();

static void do_cli_test(Span<StringView> input, Function<void(Args&)> validate_result = nullptr) {
    auto result = cli_parser.parse(input);
    if (!result) {
        auto message = format("{}", result.error());
        EXPECT_EQ(message, "Not an error");
        EXPECT(false);
    }

    validate_result.safe_call(result.value());
}

template<typename ErrorType>
static void do_cli_fail_test(Span<StringView> input, Function<void(ErrorType&)> validate_error = nullptr) {
    auto result = cli_parser.parse(input);
    EXPECT(result.is_error());
    auto& error = result.error();

    if (!error.is<ErrorType>()) {
        auto message = format("{}", result.error());
        EXPECT_EQ(message, "Different error type");
        EXPECT(false);
    }

    validate_error.safe_call(error.as<ErrorType>());
}

static void do_basic_test(Span<StringView> input) {
    do_cli_test((input), [](auto& args) {
        EXPECT_EQ(args.c, true);
        EXPECT_EQ(args.dest, "file"sv);
        EXPECT_EQ(args.i, 42);
    });
}

TEST(cli, short_flags) {
    FixedArray input = { "program"sv, "-c"sv, "-i42"sv, "file"sv };
    do_basic_test(input.span());
}

TEST(cli, combined_short_flags) {
    FixedArray input = { "program"sv, "-ci42"sv, "file"sv };
    do_cli_test(input.span(), [](auto& args) {
        EXPECT_EQ(args.c, true);
        EXPECT_EQ(args.dest, "file"sv);
        EXPECT_EQ(args.i, 42);
    });
}

TEST(cli, short_flags_separate_value) {
    FixedArray input = { "program"sv, "-i"sv, "42"sv, "-c"sv, "file"sv };
    do_basic_test(input.span());
}

TEST(cli, long_flags) {
    FixedArray input = { "program"sv, "--int=42"sv, "--crash"sv, "file"sv };
    do_basic_test(input.span());
}

TEST(cli, long_flags_separate_value) {
    FixedArray input = { "program"sv, "--crash"sv, "--int", "42"sv, "file"sv };
    do_basic_test(input.span());
}

TEST(cli, mixed_flags) {
    FixedArray input = { "program"sv, "-i42"sv, "--crash", "file"sv };
    do_basic_test(input.span());
}

TEST(cli, no_flags) {
    FixedArray input { "prgram"sv, "file"sv };
    do_cli_test(input.span(), [](auto& args) {
        EXPECT(!args.c);
        EXPECT(!args.i.has_value());
    });
}

TEST(cli, missing_positional_argument) {
    FixedArray input { "program"sv, "-c"sv };
    do_cli_fail_test<Cli::MissingPositionalArgument>(input.span(), [](auto& error) {
        EXPECT_EQ(error.argument_name(), "dest"sv);
    });
}

TEST(cli, extra_positional_argument) {
    FixedArray input { "program"sv, "-c"sv, "a"sv, "b"sv };
    do_cli_fail_test<Cli::UnexpectedPositionalArgument>(input.span(), [](auto& error) {
        EXPECT_EQ(error.value(), "b"sv);
    });
}

TEST(cli, missing_short_flag_value) {
    FixedArray input { "program"sv, "file"sv, "-i"sv };
    do_cli_fail_test<Cli::ShortFlagRequiresValue>(input.span(), [](auto& error) {
        EXPECT_EQ(error.flag(), 'i');
    });
}

TEST(cli, missing_long_flag_value) {
    FixedArray input { "program"sv, "file"sv, "--int"sv };
    do_cli_fail_test<Cli::LongFlagRequiresValue>(input.span(), [](auto& error) {
        EXPECT_EQ(error.flag(), "int"sv);
    });
}

TEST(cli, extra_short_flag) {
    FixedArray input { "program"sv, "-x"sv, "file"sv };
    do_cli_fail_test<Cli::UnexpectedShortFlag>(input.span(), [](auto& error) {
        EXPECT_EQ(error.flag(), 'x');
    });
}

TEST(cli, extra_long_flag) {
    FixedArray input { "program"sv, "--xxx"sv, "file"sv };
    do_cli_fail_test<Cli::UnexpectedLongFlag>(input.span(), [](auto& error) {
        EXPECT_EQ(error.flag(), "xxx"sv);
    });
}
