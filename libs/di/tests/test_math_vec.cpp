#include <di/assert/assert_binary.h>
#include <di/format/to_string.h>
#include <di/math/linalg/vec.h>
#include <di/util/get.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>
#include <di/vocab/tuple/tie.h>
#include <di/vocab/tuple/tuple_like.h>
#include <dius/test/prelude.h>

namespace math_vec {
struct IntTag {
    using Type = i32;
    constexpr static usize extent = 2ZU;
};

using Ints = di::math::linalg::Vec<IntTag>;

constexpr void basic() {
    auto p = Ints(1, 2);
    ASSERT_EQ(1, di::get<0>(p));
    ASSERT_EQ(1, p.get<0>());

    auto [x, y] = p;
    ASSERT_EQ(x, 1);
    ASSERT_EQ(y, 2);

    auto p2 = p + Ints(3, 5);
    p2 += 1;
    di::tie(x, y) = p2;
    ASSERT_EQ(x, 5);
    ASSERT_EQ(y, 8);

    p2 -= 5;
    ASSERT(p2 == Ints(0, 3));
}

constexpr void format() {
    auto p = Ints(1, 2);
    auto s = di::to_string(p);

    ASSERT_EQ(s, "{ 1, 2 }"_sv);
}

TESTC(math_vec, basic)
TESTC(math_vec, format)
}
