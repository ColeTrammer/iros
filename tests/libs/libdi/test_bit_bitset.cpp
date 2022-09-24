#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto set = di::BitSet<13> {};
    set[2] = true;
    set[10] = true;
    ASSERT_EQ(set[2], true);
    ASSERT_EQ(set[10], true);
}

TEST_CONSTEXPR(bit_bitset, basic, basic)