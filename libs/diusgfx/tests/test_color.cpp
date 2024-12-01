#include <di/assert/assert_binary.h>
#include <dius/test/prelude.h>
#include <diusgfx/color.h>

namespace gfx_color {
constexpr static void basic() {
    auto p = gfx::Color(255, 0, 0);
    auto q = p.with_green(123);

    ASSERT_NOT_EQ(p, q);
    ASSERT_EQ(p.alpha(), 255);
}

TESTC(gfx_color, basic)
}
