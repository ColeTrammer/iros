#include <di/math/bigint/prelude.h>
#include <dius/test/prelude.h>

namespace math_fixed_bigint {
constexpr static void unsigned_() {
    auto x = di::u256(0x1234567890abcdef);
    auto y = di::u256(0x1234567890abcdef);

    ASSERT_EQ(x / y, di::u256(1));
    ASSERT_EQ(x % y, di::u256(0));
}

constexpr static void signed_() {
    auto x = di::i256(0x1234567890abcdef);
    auto y = di::i256(0x1234567890abcdef);
    auto z = di::i256(0x1234567890abcdee);

    ASSERT_EQ(x / y, di::i256(1));
    ASSERT_EQ(x % z, di::i256(1));

    ASSERT_EQ(x / -y, -di::i256(1));
    ASSERT_EQ(x % -z, di::i256(1));

    ASSERT_EQ(-x / y, -di::i256(1));
    ASSERT_EQ(-x % z, -di::i256(1));

    ASSERT_EQ(-x / -y, di::i256(1));
    ASSERT_EQ(-x % -z, -di::i256(1));
}

TESTC(math_fixed_bigint, unsigned_)
TESTC(math_fixed_bigint, signed_)
}
