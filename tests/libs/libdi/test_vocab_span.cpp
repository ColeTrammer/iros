#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = di::to_array({ 1, 2, 3, 4, 5 });
    auto y = di::Span { x };

    EXPECT_EQ(y[0], x[0]);
    EXPECT_EQ(y.front(), 1);

    auto z = di::Span<int> { x };
    EXPECT_EQ(z.size(), 5u);
    EXPECT_EQ(z[3], x[3]);
    EXPECT_EQ(*z.front(), 1);
    EXPECT_EQ(*z.back(), 5);

    auto a = z.subspan(1, 2);
    EXPECT_EQ((*a)[0], 2);
    EXPECT_EQ((*a)[1], 3);

    auto b = z.last(2);
    EXPECT_EQ((*b)[0], 4);
    EXPECT_EQ((*b)[1], 5);
}

constexpr void tuple() {
    int array[] = { 6, 7, 8 };
    auto x = di::Span<int> { array };

    auto a = x.first<1>();
    auto [g] = *a;
    EXPECT_EQ(g, 6);

    auto b = x.subspan<1, 2>();
    static_assert(di::SameAs<decltype(*b), di::Span<int, 2>&>);

    auto [h, i] = *b;
    EXPECT_EQ(h, 7);
    EXPECT_EQ(i, 8);
}

TEST_CONSTEXPR(vocab_span, basic, basic)
TEST_CONSTEXPR(vocab_span, tuple, tuple)
