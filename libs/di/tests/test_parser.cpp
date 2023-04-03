#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace parser {
constexpr void set() {
    auto pred = U'A'_m - U'Z'_m;

    ASSERT(pred(U'B'));
    ASSERT(!pred(U'0'));

    auto alpha_num = 'a'_m - 'z'_m || 'A'_m - 'Z'_m || '0'_m - '9'_m;
    ASSERT(alpha_num(U'3'));
    ASSERT(!alpha_num(U'-'));

    auto not_alpha_num = ~alpha_num;
    ASSERT(!not_alpha_num(U'3'));
    ASSERT(not_alpha_num(U'-'));
}

constexpr void code_point() {
    ASSERT(!di::parse<c32>(u8""_sv));
    ASSERT_EQ(*di::parse<c32>(u8"A"_sv), U'A');
    ASSERT(!di::parse<c32>(u8"AB"_sv));

    ASSERT(!di::parse_partial<c32>(u8""_sv));
    ASSERT_EQ(*di::parse_partial<c32>(u8"A"_sv), U'A');
    ASSERT_EQ(*di::parse_partial<c32>(u8"AB"_sv), U'A');
}

constexpr void integer() {
    ASSERT_EQ(*di::parse<i32>(u8"0"_sv), 0);
    ASSERT(!di::parse<i32>(u8"0qwer"_sv));
    ASSERT_EQ(*di::parse<i32>(u8"123"_sv), 123);
    ASSERT_EQ(*di::parse<i32>(u8"-123"_sv), -123);

    ASSERT(!di::parse<u32>(u8"-123"_sv));
    ASSERT_EQ(di::parse<u32>(u8"+123"_sv), 123u);

    ASSERT_EQ(di::parse<i32>(u8"2147483647"_sv), di::NumericLimits<i32>::max);
    ASSERT_EQ(di::parse<i32>(u8"-2147483648"_sv), di::NumericLimits<i32>::min);
    ASSERT(!di::parse<i32>(u8"2147483648"_sv));
    ASSERT(!di::parse<i32>(u8"-2147483649"_sv));
    ASSERT(!di::parse<i32>(u8"1111111111111111111111"_sv));

    ASSERT_EQ(di::parse<u32>(u8"4294967295"_sv), di::NumericLimits<u32>::max);
    ASSERT(!di::parse<u32>(u8"4294967296"_sv));

    ASSERT_EQ(di::parse<u8>(u8"255"_sv), 255u);
    ASSERT(!di::parse<u8>(u8"256"_sv));
}

constexpr void integral_constant() {
    ASSERT_EQ(1_zic, 1u);
    ASSERT_EQ(4161_zic, 4161u);
}

constexpr void alternation() {
    auto parser = di::parser::integer<i32>() | di::parser::match_one('='_m);

    ASSERT_EQ(*di::run_parser(parser, "128"_sv), 128);
    ASSERT_EQ(*di::run_parser(parser, "="_sv), U'=');
    ASSERT(!di::run_parser(parser, "!"_sv));
}

TESTC(parser, set)
TESTC(parser, code_point)
TESTC(parser, integer)
TESTC(parser, integral_constant)
TESTC(parser, alternation)
}
