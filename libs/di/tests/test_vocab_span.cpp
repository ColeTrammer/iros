#include "di/vocab/array/prelude.h"
#include "di/vocab/span/prelude.h"
#include "dius/test/prelude.h"

namespace vocab_span {
constexpr static void basic() {
    auto x = di::to_array({ 1, 2, 3, 4, 5 });
    auto y = di::Span { x };

    ASSERT_EQ(y[0], x[0]);
    ASSERT_EQ(y.front(), 1);

    auto z = di::Span<int> { x };
    ASSERT_EQ(z.size(), 5U);
    ASSERT_EQ(z[3], x[3]);
    ASSERT_EQ(*z.front(), 1);
    ASSERT_EQ(*z.back(), 5);

    auto a = z.subspan(1, 2);
    ASSERT_EQ((*a)[0], 2);
    ASSERT_EQ((*a)[1], 3);

    auto b = z.last(2);
    ASSERT_EQ((*b)[0], 4);
    ASSERT_EQ((*b)[1], 5);

    auto c = y.to_owned();
    ASSERT_EQ(c, x);
}

constexpr static void tuple() {
    int array[] = { 6, 7, 8 };
    auto x = di::Span<int> { array };

    auto a = x.first<1>();
    auto [g] = *a;
    ASSERT_EQ(g, 6);

    auto b = x.subspan<1, 2>();
    static_assert(di::SameAs<decltype(*b), di::Span<int, 2>&>);

    auto [h, i] = *b;
    ASSERT_EQ(h, 7);
    ASSERT_EQ(i, 8);
}

TESTC(vocab_span, basic)
TESTC(vocab_span, tuple)
}
