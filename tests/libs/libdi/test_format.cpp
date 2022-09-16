#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    static_assert(di::Formattable<int>);

    auto s = di::present("{}"_sv, 42);
    EXPECT(s.view() == "42"_sv);
    EXPECT(s.view() != "43"_sv);

    auto t = di::present("{}{}"_sv, 42, 42);
    EXPECT(t.view() == "4242"_sv);
    EXPECT(t.view() != "4243"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
