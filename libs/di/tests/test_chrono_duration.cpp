#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void basic() {
    auto x = 100_ms;
    x += 50_ms;

    ASSERT_EQ(x, 150_ms);

    x += 1_s;

    ASSERT_EQ(x, 1150_ms);

    auto y = di::duration_cast<di::Seconds>(x);
    ASSERT_EQ(y, 1000_ms);
}

TESTC(chrono_duration, basic)
