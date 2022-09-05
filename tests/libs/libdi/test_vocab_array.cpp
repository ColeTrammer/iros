#include <di/prelude.h>
#include <test/test.h>

constexpr void to_array() {
    {
        int array[] = { 1, 2, 3, 4, 5 };
        auto x = di::to_array(array);
        EXPECT_EQ(x[0], 1);
        EXPECT_EQ(x[4], 5);
    }

    {
        auto x = di::to_array({ 1, 2, 3, 4, 5 });
        EXPECT_EQ(x[0], 1);
        EXPECT_EQ(x[4], 5);
    }
}

constexpr void span() {
    {
        auto x = di::to_array({ 1, 2, 3, 4 });
        auto [a, b] = x.first<2>();
        EXPECT_EQ(a, 1);
        EXPECT_EQ(b, 2);

        auto const y = x.last<1>();
        EXPECT_EQ(y.front(), 4);
        auto [c] = y;

        static_assert(di::SameAs<di::meta::TupleElement<di::Span<int, 2>, 0>, int>);
        static_assert(di::SameAs<int, decltype(a)>);
        static_assert(di::SameAs<int, decltype(b)>);
        static_assert(di::SameAs<int, decltype(c)>);

        static_assert(di::SameAs<int, std::tuple_element<0, di::Span<int, 2>>::type>);
    }
}

constexpr void tuple() {
    auto x = di::Array { 1, 2, 3 };
    auto [a, b, c] = x;
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);

    static_assert(di::SameAs<int, decltype(a)>);
    static_assert(di::concepts::TupleLike<decltype(x)>);
}

TEST_CONSTEXPR(vocab_array, to_array, to_array)
TEST_CONSTEXPR(vocab_array, span, span)
TEST_CONSTEXPR(vocab_array, tuple, tuple)
