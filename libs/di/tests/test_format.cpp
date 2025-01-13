#include "di/format/prelude.h"
#include "di/math/prelude.h"
#include "dius/test/prelude.h"

namespace format {
constexpr static void basic() {
    auto s = di::present(u8"a{}"_sv, 42);
    ASSERT_EQ(s, u8"a42"_sv);
    ASSERT_NOT_EQ(s, u8"43"_sv);

    auto t = di::present(u8"b{}b{}b"_sv, 42, 42);
    ASSERT_EQ(t, u8"b42b42b"_sv);
    ASSERT_NOT_EQ(t, u8"4243"_sv);

    auto q = di::present(u8"{{{}"_sv, -42);
    ASSERT_EQ(q, u8"{-42"_sv);

    auto u = di::present(u8"{{{}}}"_sv, 153U);
    ASSERT_EQ(u, u8"{153}"_sv);

    auto a = di::present(u8"{}"_sv, di::Array { 1, 2, 3 });
    ASSERT_EQ(a, u8"{ 1, 2, 3 }"_sv);

    auto b = di::present("{0:} {0:} {0:} {1:}"_sv, 1, 2);
    ASSERT_EQ(b, "1 1 1 2"_sv);

    auto c = di::present("{:.1}"_sv, "xxx"_sv);
    ASSERT_EQ(c, "x"_sv);

    ASSERT_EQ(di::present("{:*>10s}"_sv, "HELLO"_sv), "*****HELLO"_sv);
    ASSERT_EQ(di::present("{:*^10s}"_sv, "HELLO"_sv), "**HELLO***"_sv);
    ASSERT_EQ(di::present("{:*<10s}"_sv, "HELLO"_sv), "HELLO*****"_sv);

    ASSERT_EQ(di::present("{:5c}"_sv, U'a'), "a    "_sv);
    ASSERT_EQ(di::present("{}"_sv, false), "false"_sv);

    ASSERT_EQ(di::present("{:#x}"_sv, 0x1234fe), "0x1234fe"_sv);
    ASSERT_EQ(di::present("{:#X}"_sv, 0x1234fe), "0X1234FE"_sv);
    ASSERT_EQ(di::present("{:#b}"_sv, 0b10101), "0b10101"_sv);
    ASSERT_EQ(di::present("{:#6x}"_sv, 0x01), "   0x1"_sv);
    ASSERT_EQ(di::present("{}"_sv, 0), "0"_sv);
    ASSERT_EQ(di::present("{:+}"_sv, 0), "+0"_sv);
    ASSERT_EQ(di::present("{: }"_sv, 0), " 0"_sv);
    ASSERT_EQ(di::present("{:#04x}"_sv, 0x01), "0x01"_sv);
    ASSERT_EQ(di::present("{}"_sv, di::NumericLimits<i8>::min), "-128"_sv);
    ASSERT_EQ(di::present("{}"_sv, di::NumericLimits<i32>::min), "-2147483648"_sv);
    ASSERT_EQ(di::present("{}"_sv, di::NumericLimits<i32>::max), "2147483647"_sv);
    ASSERT_EQ(di::present("{}"_sv, di::NumericLimits<u32>::max), "4294967295"_sv);

    ASSERT_EQ(di::present("{:?}"_sv, "abc"_sv), "\"abc\""_sv);
    ASSERT_EQ(di::present("{:?}"_sv, U'x'), "'x'"_sv);
    ASSERT_EQ(di::present("{}"_sv, 42_b), "42"_sv);
    ASSERT_EQ(di::present("{}"_sv, U'界'), u8"界"_sv);

    ASSERT_EQ(di::present("{}"_sv, di::Styled(42, di::FormatColor::Red | di::FormatEffect::Bold)), "42"_sv);
}

TESTC(format, basic)
}
