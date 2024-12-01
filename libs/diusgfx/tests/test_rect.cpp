#include <di/assert/assert_binary.h>
#include <dius/test/prelude.h>
#include <diusgfx/point.h>
#include <diusgfx/rect.h>

namespace gfx_rect {
constexpr static void basic() {
    auto rect = gfx::Rect(0, 0, 50, 50);
    ASSERT_EQ(rect.x(), 0);
    ASSERT_EQ(rect.width(), 50);

    ASSERT_EQ(rect.top_left(), gfx::Point(0, 0));
    ASSERT_EQ(rect.top_right(), gfx::Point(50, 0));
    ASSERT_EQ(rect.bottom_left(), gfx::Point(0, 50));
    ASSERT_EQ(rect.bottom_right(), gfx::Point(50, 50));

    ASSERT(rect.contains({ 25, 25 }));
    ASSERT(!rect.contains({ 25, 50.001 }));

    ASSERT_EQ(rect.center(), gfx::Point(25, 25));
}

TESTC(gfx_rect, basic)
}
