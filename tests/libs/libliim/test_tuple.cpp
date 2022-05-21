#include <liim/pointers.h>
#include <liim/tuple.h>
#include <test/test.h>
#include <tuple>

constexpr void basic() {
    Tuple<int, long> x;
    EXPECT_EQ(x.get<0>(), 0);
    EXPECT_EQ(x.get<1>(), 0);

    x = { 3, 5l };
    EXPECT_EQ(x.get<0>(), 3);
    EXPECT_EQ(x.get<1>(), 5l);

    Tuple<int, UniquePtr<int>> y(5, make_unique<int>(10));
    EXPECT_EQ(y.get<0>(), 5);
    EXPECT_EQ(*y.get<1>(), 10);

    auto w = move(y).get<1>();
}

constexpr void destructing() {
    auto x = Tuple { 5, 7l };
    auto& [i, l] = x;
    EXPECT_EQ(i, 5);
    EXPECT_EQ(l, 7l);
    i = 4;
    EXPECT_EQ(x.get<0>(), 4);

    auto [a, b] = Tuple<int, UniquePtr<int>> { 8, make_unique<int>(15) };
    EXPECT_EQ(a, 8);
    EXPECT_EQ(*b, 15);

    auto z = Tuple<int&, long&> { x.get<0>(), x.get<1>() };
    auto c = z;
    EXPECT_EQ(c.get<0>(), 4);
    EXPECT_EQ(c.get<1>(), 7l);
}

TEST_CONSTEXPR(tuple, basic, basic)
TEST_CONSTEXPR(tuple, destructing, destructing)
