#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    static_assert(di::Formattable<int>);

    auto s = di::present("a{}"_sv, 42);
    ASSERT_EQ(s.view(), "a42"_sv);
    ASSERT_NOT_EQ(s.view(), "43"_sv);

    auto t = di::present("b{}b{}b"_sv, 42, 42);
    ASSERT_EQ(t.view(), "b42b42b"_sv);
    ASSERT_NOT_EQ(t.view(), "4243"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
