#include <di/prelude.h>
#include <test/test.h>

constexpr void minmax() {
    ASSERT_EQ(di::min(1, 2), 1);
    ASSERT_EQ(di::min({ 5, 4, 3, 2, 1 }), 1);
    ASSERT_EQ(di::min(di::range(5, 10)), 5);

    ASSERT_EQ(di::max(1, 2), 2);
    ASSERT_EQ(di::max({ 5, 4, 3, 2, 1 }), 5);
    ASSERT_EQ(di::max(di::range(5, 10)), 9);

    ASSERT_EQ(di::clamp(2, 5, 7), 5);
    ASSERT_EQ(di::clamp(6, 5, 7), 6);
    ASSERT_EQ(di::clamp(10, 5, 7), 7);
}

constexpr void compare() {
    auto a = di::range(6);
    auto b = di::Array { 0, 1, 2, 3, 4, 5 };
    auto c = di::InitializerList { 0, 1, 2 };
    auto d = di::range(12);
    auto e = di::range(6, 12);

    ASSERT(di::container::equal(a, b));
    ASSERT(!di::container::equal(a, c));
    ASSERT(!di::container::equal(a, d));
    ASSERT(!di::container::equal(a, e));

    ASSERT(di::container::compare(a, b) == 0);
    ASSERT(di::container::compare(a, c) > 0);
    ASSERT(di::container::compare(a, d) < 0);
    ASSERT(di::container::compare(a, e) < 0);
}

constexpr void fold() {
    auto a = di::range(6);
    ASSERT_EQ(di::sum(a), 15);
    ASSERT_EQ(di::fold_left(a | di::drop(1), 1,
                            [](int acc, int x) {
                                return acc * x;
                            }),
              120);
}

constexpr void permute() {
    auto a = di::Array { 1, 2, 3, 4, 5 };
    auto b = a;
    di::container::reverse(a);
    ASSERT(di::container::equal(a, b | di::view::reverse));

    di::container::reverse(a);
    di::container::rotate(a, a.begin() + 2);
    ASSERT_EQ(a, (di::Array { 3, 4, 5, 1, 2 }));
}

constexpr void contains() {
    auto a = di::range(5);
    auto b = di::range(3);
    auto c = di::range(3, 5);
    ASSERT(di::starts_with(a, b));
    ASSERT(di::ends_with(a, c));

    ASSERT(di::contains(a, 4));
    ASSERT(!di::contains(a, 5));

    ASSERT(di::contains_subrange(a, di::range(1, 3)));
    ASSERT(!di::contains_subrange(a, di::range(6)));
    ASSERT(!di::contains_subrange(a, di::range(3, 19)));
}

TEST_CONSTEXPR(container_algorithm, minmax, minmax)
TEST_CONSTEXPR(container_algorithm, compare, compare)
TEST_CONSTEXPR(container_algorithm, fold, fold)
TEST_CONSTEXPR(container_algorithm, permute, permute)
TEST_CONSTEXPR(container_algorithm, contains, contains)