#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto s = di::present(u8"a{}"_sv, 42);
    ASSERT_EQ(s.view(), u8"a42"_sv);
    ASSERT_NOT_EQ(s.view(), u8"43"_sv);

    auto t = di::present(u8"b{}b{}b"_sv, 42, 42);
    ASSERT_EQ(t.view(), u8"b42b42b"_sv);
    ASSERT_NOT_EQ(t.view(), u8"4243"_sv);

    auto q = di::present(u8"{}"_sv, -42);
    ASSERT_EQ(q, u8"-42"_sv);

    auto u = di::present(u8"{}"_sv, 153u);
    ASSERT_EQ(u, u8"153"_sv);

    auto a = di::present(u8"{}"_sv, di::Array { 1, 2, 3 });
    ASSERT_EQ(a, u8"{ 1, 2, 3 }"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
