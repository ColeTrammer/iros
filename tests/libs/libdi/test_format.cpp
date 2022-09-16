#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    static_assert(di::Formattable<int>);

    auto s = di::present("a{}"_sv, 42);
    EXPECT(s.view() == "a42"_sv);
    EXPECT(s.view() != "43"_sv);

    auto t = di::present("b{}b{}b"_sv, 42, 42);
    EXPECT(t.view() == "b42b42b"_sv);
    EXPECT(t.view() != "4243"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
