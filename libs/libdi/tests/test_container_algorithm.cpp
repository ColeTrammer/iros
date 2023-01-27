#include <di/prelude.h>
#include <dius/test/prelude.h>

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

    {
        auto [a, b] = di::minmax({ 1, 1, 2, 2, 3, 3, 4, 4, 4 });
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 4);
    }
    {
        auto [a, b] = di::minmax({ 1 });
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 1);
    }
    {
        auto [a, b] = di::minmax({ 1, 2 });
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 2);
    }
    {
        auto [a, b] = di::minmax({ 4, 3, 2, 1 });
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 4);
    }
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
    ASSERT_EQ(di::fold_left(a | di::drop(1), 1, di::multiplies), 120);

    auto e = di::range(6) | di::to<di::Vector>();
    ASSERT_EQ(di::sum(e), 15);

    ASSERT_EQ(*di::fold_left_first(a | di::drop(1), di::multiplies), 120);
}

constexpr void is_sorted() {
    auto a = di::Array { 1, 2, 3, 4, 5 };
    ASSERT(di::is_sorted(a));

    auto b = di::Array { 1, 2, 3, 4, 3 };
    ASSERT(!di::is_sorted(b));
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

constexpr void predicate() {
    ASSERT(di::all_of(di::range(5), di::curry_back(di::less)(5)));
    ASSERT(di::none_of(di::range(5), di::curry_back(di::greater)(4)));
    ASSERT(di::any_of(di::range(5), di::curry(di::equal)(4)));

    ASSERT_EQ(di::count(di::range(5), 3), 1);
    ASSERT_EQ(di::count_if(di::range(5),
                           [](int x) {
                               return x == 2 || x == 3;
                           }),
              2);
}

constexpr void for_each() {
    int sum = 0;
    di::for_each(di::range(6), [&](int x) {
        sum += x;
    });
    ASSERT_EQ(sum, 15);
}

constexpr void sort() {
    auto v = di::Array { 3, 5, 1, 2 };
    di::sort(v);
    ASSERT_EQ(v, (di::Array { 1, 2, 3, 5 }));

    auto w = di::Array { 8, 7, 6, 5, 4, 3, 2, 1, 0 } | di::to<di::Vector>();
    di::sort(w);
    ASSERT_EQ(w, di::range(9) | di::to<di::Vector>());

    di::sort(w, di::compare_backwards);
    ASSERT_EQ(w, di::range(9) | di::reverse | di::to<di::Vector>());

    auto x = di::range(128) | di::to<di::Vector>();
    di::sort(x, di::compare_backwards);
    ASSERT_EQ(x, di::range(128) | di::reverse | di::to<di::Vector>());
    ASSERT(di::is_sorted(x | di::reverse));

    auto y = di::range(56) | di::to<di::Vector>();
    y.append_container(di::range(56));
    di::sort(y);
    ASSERT(di::is_sorted(y));

    auto scores = di::Array { 50, 20, 70, 10 };
    auto data = di::Array { 37, 42, 60, 100, -1 };
    di::sort(di::zip(scores, data));

    ASSERT(di::is_sorted(di::zip(scores, data)));
    ASSERT_EQ(scores, (di::Array { 10, 20, 50, 70 }));
    ASSERT_EQ(data, (di::Array { 100, 42, 37, 60, -1 }));

    struct X {
        int a;
    };
    auto s = di::Array { X { 5 }, X { 4 }, X { 2 }, X { 3 } };
    di::sort(s, di::compare, &X::a);
    ASSERT(di::is_sorted(s, di::compare, &X::a));
}

TESTC(container_algorithm, minmax)
TESTC(container_algorithm, compare)
TESTC(container_algorithm, fold)
TESTC(container_algorithm, is_sorted)
TESTC(container_algorithm, permute)
TESTC(container_algorithm, contains)
TESTC(container_algorithm, predicate)
TESTC(container_algorithm, for_each)
TESTC(container_algorithm, sort)