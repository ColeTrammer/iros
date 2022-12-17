#include <di/prelude.h>
#include <test/test.h>

constexpr void set() {
    auto pred = U'A'_m - U'Z'_m;

    ASSERT(pred(U'B'));
    ASSERT(!pred(U'0'));

    auto alpha_num = 'a'_m - 'z'_m || 'A'_m - 'Z'_m || '0'_m - '9'_m;
    ASSERT(alpha_num(U'3'));
    ASSERT(!alpha_num(U'-'));

    auto not_alpha_num = !alpha_num;
    ASSERT(!not_alpha_num(U'3'));
    ASSERT(not_alpha_num(U'-'));
}

constexpr void code_point() {
    ASSERT(!di::parse<char32_t>(""_sv));
    ASSERT_EQ(*di::parse<char32_t>("A"_sv), U'A');
    ASSERT(!di::parse<char32_t>("AB"_sv));

    ASSERT(!di::parse_partial<char32_t>(""_sv));
    ASSERT_EQ(*di::parse_partial<char32_t>("A"_sv), U'A');
    ASSERT_EQ(*di::parse_partial<char32_t>("AB"_sv), U'A');
}

constexpr void integer() {
    ASSERT_EQ(*di::parse<i32>("0"_sv), 0);
    ASSERT(!di::parse<i32>("0qwer"_sv));
    ASSERT_EQ(*di::parse<i32>("123"_sv), 123);
    ASSERT_EQ(*di::parse<i32>("-123"_sv), -123);

    ASSERT(!di::parse<u32>("-123"_sv));
    ASSERT_EQ(di::parse<u32>("+123"_sv), 123u);
}

TEST_CONSTEXPR(parser, set, set)
TEST_CONSTEXPR(parser, code_point, code_point)
TEST_CONSTEXPR(parser, integer, integer)