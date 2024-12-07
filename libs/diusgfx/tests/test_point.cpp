#include "di/assert/assert_binary.h"
#include "dius/test/prelude.h"
#include "diusgfx/point.h"

namespace gfx_point {
constexpr static void basic() {
    auto p = gfx::Point(1, 2);
    p.x() += 2;

    ASSERT_EQ(p, gfx::Point(3, 2));

    auto q = p.with_y(5);
    ASSERT_EQ(q, gfx::Point(3, 5));
}

TESTC(gfx_point, basic)
}
