#include "di/vocab/array/prelude.h"
#include "dius/test/prelude.h"

namespace vocab_array {
constexpr static void to_array() {
    {
        int array[] = { 1, 2, 3, 4, 5 };
        auto x = di::to_array(array);
        ASSERT_EQ(x[0], 1);
        ASSERT_EQ(x[4], 5);
    }

    {
        auto x = di::to_array({ 1, 2, 3, 4, 5 });
        ASSERT_EQ(x[0], 1);
        ASSERT_EQ(x[4], 5);
    }
}

constexpr static void span() {
    {
        auto x = di::to_array({ 1, 2, 3, 4 });
        auto [a, b] = x.first<2>();
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 2);

        auto const y = x.last<1>();
        ASSERT_EQ(y.front(), 4);
        auto [c] = y;

        static_assert(di::SameAs<di::meta::TupleElement<di::Span<int, 2>, 0>, int>);
        static_assert(di::SameAs<int, decltype(a)>);
        static_assert(di::SameAs<int, decltype(b)>);
        static_assert(di::SameAs<int, decltype(c)>);

        static_assert(di::SameAs<int, std::tuple_element<0, di::Span<int, 2>>::type>);
    }
}

constexpr static void tuple() {
    auto x = di::Array { 1, 2, 3 };
    auto [a, b, c] = x;
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);

    static_assert(di::SameAs<int, decltype(a)>);
    static_assert(di::concepts::TupleLike<decltype(x)>);
}

constexpr static void compare() {
    auto a = di::to_array({ 1, 2, 3 });
    auto b = di::to_array({ 1, 2, 4 });
    ASSERT_NOT_EQ(a, b);
    ASSERT_LT(a, b);
}

TESTC(vocab_array, to_array)
TESTC(vocab_array, span)
TESTC(vocab_array, tuple)
TESTC(vocab_array, compare)
}
