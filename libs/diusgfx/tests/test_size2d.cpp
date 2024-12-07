#include "di/assert/assert_binary.h"
#include "dius/test/prelude.h"
#include "diusgfx/size2d.h"

namespace gfx_Size2d {
constexpr static void basic() {
    auto p = gfx::Size2d(1, 2);
    p.width() += 2;

    ASSERT_EQ(p, gfx::Size2d(3, 2));

    auto q = p.with_height(5);
    ASSERT_EQ(q, gfx::Size2d(3, 5));
}

TESTC(gfx_size2d, basic)
}
