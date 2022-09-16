#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    static_assert(di::Formattable<int>);

    auto format = di::format::FormatStringImpl<di::container::string::TransparentEncoding, int>("{}"_sv);
    auto s = di::present(format, 42);
    EXPECT(s.view() == "42"_sv);
    EXPECT(s.view() != "43"_sv);

    auto tformat = di::format::FormatStringImpl<di::container::string::TransparentEncoding, int, int>("{}{}"_sv);
    auto t = di::present(tformat, 42, 42);
    EXPECT(t.view() == "4242"_sv);
    EXPECT(t.view() != "4243"_sv);
}

TEST_CONSTEXPR(format, basic, basic)
