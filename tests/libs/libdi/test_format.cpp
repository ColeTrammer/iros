#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto s = di::present("xx"_sv);
    EXPECT(s.empty());
}

TEST_CONSTEXPR(format, basic, basic)
