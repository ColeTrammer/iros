#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = 100_ms;
    x += 50_ms;

    ASSERT_EQ(x, 150_ms);

    x += 1_s;

    ASSERT_EQ(x, 1150_ms);
}

TEST_CONSTEXPR(chrono_duration, basic, basic)