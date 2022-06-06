#include <liim/new_vector.h>
#include <liim/pair.h>
#include <test/test.h>

constexpr void basic() {
    Pair<int, float> x;

    Pair<int, float> y;
    x = y;

    auto z = make_pair(x.first, y.first);
    EXPECT_EQ(z.first, 0);
    EXPECT_EQ(z.second, 0);

    EXPECT(x == y);

    Pair<int, int> a(4, 6);
    Pair<int, int> b(3, 8);
    EXPECT(b < a);

    Pair<short, short> c(3, 6);
    Pair<int, int> d = c;
    EXPECT_EQ(d.first, 3);
    EXPECT_EQ(d.first, 3);
    EXPECT_EQ(d.second, 6);

    auto [e, f] = d;
    EXPECT_EQ(e, 3);
    EXPECT_EQ(f, 6);

    // FIXME: make pair "create" aware
    // Pair<NewVector<int>, int> www(LIIM::piecewise_construct, Tuple { 5, 3 }, Tuple { 4 });
}

TEST_CONSTEXPR(pair, basic, basic)
