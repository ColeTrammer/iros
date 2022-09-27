#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto s = di::present("a{}"_sv, 42);
    ASSERT_EQ(s.view(), "a42"_sv);
    ASSERT_NOT_EQ(s.view(), "43"_sv);

    auto t = di::present("b{}b{}b"_sv, 42, 42);
    ASSERT_EQ(t.view(), "b42b42b"_sv);
    ASSERT_NOT_EQ(t.view(), "4243"_sv);

    auto q = di::present("{}"_sv, -42);
    ASSERT_EQ(q, "-42"_sv);

    auto u = di::present("{}"_sv, 153u);
    ASSERT_EQ(u, "153"_sv);

    auto a = di::present("{}"_sv, di::Array { 1, 2, 3 });
    ASSERT_EQ(a, "{ 1, 2, 3 }"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
