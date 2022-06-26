#include <liim/container/array.h>
#include <liim/tuple.h>
#include <test/test.h>

constexpr void basic() {
    auto array = make_array({ 1, 5, 6, 2 });
    EXPECT_EQ(array.size(), 4u);
    EXPECT_EQ(array[0], 1);
    EXPECT_EQ(array.at(4), None {});
    EXPECT_EQ(array.front(), 1);
    EXPECT_EQ(array.back(), 2);

    array.fill(3);
    EXPECT_EQ(array, (Array { 3, 3, 3, 3 }));
}

constexpr void destructured() {
    auto array = make_array({ 6, 2, 1 });
    auto [x, y, z] = array;
    EXPECT_EQ(x, 6);
    EXPECT_EQ(y, 2);
    EXPECT_EQ(z, 1);

    auto f = [](auto x, auto y, auto z) {
        return x + y + z;
    };
    EXPECT_EQ(tuple_apply(f, array), 9);
}

TEST_CONSTEXPR(array, basic, basic)
TEST_CONSTEXPR(array, destructured, destructured)
