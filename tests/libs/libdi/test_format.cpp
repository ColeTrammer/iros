#include <di/prelude.h>
#include <test/test.h>

#include <dius/prelude.h>

constexpr void basic() {
    auto s = di::present(u8"a{}"_sv, 42);
    ASSERT_EQ(s, u8"a42"_sv);
    ASSERT_NOT_EQ(s, u8"43"_sv);

    auto t = di::present(u8"b{}b{}b"_sv, 42, 42);
    ASSERT_EQ(t, u8"b42b42b"_sv);
    ASSERT_NOT_EQ(t, u8"4243"_sv);

    auto q = di::present(u8"{{{}"_sv, -42);
    ASSERT_EQ(q, u8"{-42"_sv);

    auto u = di::present(u8"{{{}}}"_sv, 153u);
    ASSERT_EQ(u, u8"{153}"_sv);

    auto a = di::present(u8"{}"_sv, di::Array { 1, 2, 3 });
    ASSERT_EQ(a, u8"{ 1, 2, 3 }"_sv);

    auto b = di::present("{0:} {0:} {0:} {1:}"_sv, 1, 2);
    ASSERT_EQ(b, "1 1 1 2"_sv);

    auto c = di::present("{:.1}"_sv, "xxx"_sv);
    ASSERT_EQ(c, "x"_sv);

    ASSERT_EQ(di::present("{:*>10s}"_sv, "HELLO"_sv), "HELLO*****"_sv);
    ASSERT_EQ(di::present("{:*^10s}"_sv, "HELLO"_sv), "**HELLO***"_sv);
    ASSERT_EQ(di::present("{:*<10s}"_sv, "HELLO"_sv), "*****HELLO"_sv);

    ASSERT_EQ(di::present("{:5c}"_sv, U'a'), "    a"_sv);
    ASSERT_EQ(di::present("{}"_sv, false), "false"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
