#include <dius/test/prelude.h>
#include <getopt.h>
#include <string.h>

namespace getopt_h {
static int saw_bar = 0;

constexpr auto options = di::Array {
    option { "foo", required_argument, nullptr, 'f' },
    option { "baz", no_argument, nullptr, 'b' },
    option { "bar", optional_argument, &saw_bar, 42 },
    option { nullptr, 0, nullptr, 0 },
};

constexpr auto short_options = "f:bz";
constexpr auto strict_short_options = ":f:bz";

struct GetoptResult {
    di::Optional<di::TransparentStringView> foo_value;
    bool baz { false };
    di::Optional<di::TransparentStringView> bar_value;
    di::Vector<di::TransparentStringView> arguments;
    bool got_colon_error { false };
    bool got_question_mark_error { false };
};

static void reset_getopt() {
    opterr = 0;
    optind = 0;
    saw_bar = 0;
}

enum class GetoptFunction {
    Short,
    Long,
    LongOnly,
};

static GetoptResult do_getopt(char const* short_options, di::Span<char const*> argv, GetoptFunction function) {
    auto result = GetoptResult {};
    auto opt = 0;
    auto longindex = 0;

    auto do_getopt = [&] -> int {
        switch (function) {
            case GetoptFunction::Short:
                return getopt(argv.size(), const_cast<char**>(argv.data()), short_options);
            case GetoptFunction::Long:
                return getopt_long(argv.size(), const_cast<char**>(argv.data()), short_options, options.data(),
                                   &longindex);
            case GetoptFunction::LongOnly:
                return getopt_long_only(argv.size(), const_cast<char**>(argv.data()), short_options, options.data(),
                                        &longindex);
            default:
                di::unreachable();
        }
    };

    reset_getopt();

    while ((opt = do_getopt()) != -1) {
        switch (opt) {
            case 'f':
                ASSERT(optarg);
                result.foo_value = di::TransparentStringView(optarg, strlen(optarg));
                break;
            case 'b':
                result.baz = true;
                break;
            case 0:
                ASSERT_EQ(saw_bar, 42);
                ASSERT_EQ(longindex, 2);
                [[fallthrough]];
            case 'z':
                if (optarg) {
                    result.bar_value = di::TransparentStringView(optarg, strlen(optarg));
                }
                break;
            case ':':
                result.got_colon_error = true;
                return result;
            case '?':
                result.got_question_mark_error = true;
                return result;
            default:
                ASSERT(false);
        }
    }

    for (auto i : di::range(usize(optind), argv.size())) {
        result.arguments.push_back(di::TransparentStringView(argv[i], strlen(argv[i])));
    }

    return result;
}

static void getopt_basic() {
    auto result =
        do_getopt(short_options, di::Array { "test", "-f", "bar", "-b", "-z", "z" }.span(), GetoptFunction::Short);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_combined() {
    auto result = do_getopt(short_options, di::Array { "test", "-bzf", "bar", "z" }.span(), GetoptFunction::Short);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_value() {
    auto result = do_getopt(short_options, di::Array { "test", "-fbar", "z" }.span(), GetoptFunction::Short);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_strict_invalid_argument() {
    auto result = do_getopt(strict_short_options, di::Array { "test", "-qbar" }.span(), GetoptFunction::Short);

    ASSERT_EQ(result.foo_value, di::nullopt);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, di::Vector<di::TransparentStringView> {});
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, true);
    ASSERT_EQ(optopt, 'q');
}

static void getopt_force_positional_arguments() {
    auto result = do_getopt(short_options, di::Array { "test", "a", "-", "b", "-fbar", "c", "--", "-b", "z" }.span(),
                            GetoptFunction::Short);

    auto const expected_arguments =
        di::Array { "a"_tsv, "-"_tsv, "b"_tsv, "c"_tsv, "-b"_tsv, "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_basic() {
    auto result = do_getopt(short_options, di::Array { "test", "--foo", "bar", "--baz", "--bar=baz", "z" }.span(),
                            GetoptFunction::Long);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, "baz"_tsv);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_mixed() {
    auto result = do_getopt(short_options, di::Array { "test", "--foo", "bar", "-b", "--bar=baz", "z" }.span(),
                            GetoptFunction::Long);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, "baz"_tsv);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_no_optional_value() {
    auto result = do_getopt(short_options, di::Array { "test", "--foo", "bar", "--baz", "--bar", "z" }.span(),
                            GetoptFunction::Long);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_reordering() {
    auto result =
        do_getopt(short_options, di::Array { "test", "a", "--foo", "bar", "b", "--baz", "--bar=baz", "z" }.span(),
                  GetoptFunction::Long);

    auto const expected_arguments = di::Array { "a"_tsv, "b"_tsv, "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, "baz"_tsv);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_error_colon() {
    auto result = do_getopt(strict_short_options, di::Array { "test", "--foo" }.span(), GetoptFunction::Long);

    ASSERT_EQ(result.foo_value, di::nullopt);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, di::Vector<di::TransparentStringView> {});
    ASSERT_EQ(result.got_colon_error, true);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_error_question_mark() {
    auto result = do_getopt(short_options, di::Array { "test", "--qux" }.span(), GetoptFunction::Long);

    ASSERT_EQ(result.foo_value, di::nullopt);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, di::Vector<di::TransparentStringView> {});
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, true);
}

static void getopt_long_error_question_mark_not_strict() {
    auto result = do_getopt(short_options, di::Array { "test", "--foo" }.span(), GetoptFunction::Long);

    ASSERT_EQ(result.foo_value, di::nullopt);
    ASSERT_EQ(result.baz, false);
    ASSERT_EQ(result.bar_value, di::nullopt);
    ASSERT_EQ(result.arguments, di::Vector<di::TransparentStringView> {});
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, true);
}

static void getopt_long_only_basic() {
    auto result = do_getopt(short_options, di::Array { "test", "-foo", "bar", "-baz", "--bar=baz", "z" }.span(),
                            GetoptFunction::LongOnly);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, "baz"_tsv);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

static void getopt_long_only_mixed() {
    auto result =
        do_getopt(short_options, di::Array { "test", "-bf", "bar", "--bar=baz", "z" }.span(), GetoptFunction::LongOnly);

    auto const expected_arguments = di::Array { "z"_tsv } | di::to<di::Vector>();

    ASSERT_EQ(result.foo_value, "bar"_tsv);
    ASSERT_EQ(result.baz, true);
    ASSERT_EQ(result.bar_value, "baz"_tsv);
    ASSERT_EQ(result.arguments, expected_arguments);
    ASSERT_EQ(result.got_colon_error, false);
    ASSERT_EQ(result.got_question_mark_error, false);
}

TEST(getopt_h, getopt_basic)
TEST(getopt_h, getopt_combined)
TEST(getopt_h, getopt_value)
TEST(getopt_h, getopt_strict_invalid_argument)
TEST(getopt_h, getopt_force_positional_arguments)
TEST(getopt_h, getopt_long_basic)
TEST(getopt_h, getopt_long_mixed)
TEST(getopt_h, getopt_long_no_optional_value)
TEST(getopt_h, getopt_long_reordering)
TEST(getopt_h, getopt_long_error_colon)
TEST(getopt_h, getopt_long_error_question_mark)
TEST(getopt_h, getopt_long_error_question_mark_not_strict)
TEST(getopt_h, getopt_long_only_basic)
TEST(getopt_h, getopt_long_only_mixed)
}
