#include <di/assert/assert_binary.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/interface/access.h>
#include <di/container/string/string_view.h>
#include <di/container/tree/tree_map.h>
#include <di/container/vector/vector.h>
#include <di/container/view/prelude.h>
#include <dius/test/prelude.h>

namespace container_algorithm {
constexpr void access() {
    auto v = di::Array { 1, 2, 3, 4, 5 };
    auto v2 = di::Vector<i32> {};

    ASSERT_EQ(di::front(v), 1);
    ASSERT_EQ(di::back(v), 5);
    ASSERT_EQ(di::front_unchecked(v), 1);
    ASSERT_EQ(di::back_unchecked(v), 5);

    ASSERT_EQ(di::front(v2), di::nullopt);
    ASSERT_EQ(di::back(v2), di::nullopt);

    ASSERT_EQ(v | di::at(0), 1);
    ASSERT_EQ(v | di::at(4), 5);
    ASSERT_EQ(v | di::at(5), di::nullopt);
    ASSERT_EQ(v | di::at(-1), di::nullopt);

    ASSERT_EQ(v | di::at_unchecked(0), 1);
    ASSERT_EQ(v | di::at_unchecked(4), 5);

    auto m = di::TreeMap<di::StringView, di::StringView> {};
    m.insert({ "a"_sv, "b"_sv });
    m.insert({ "c"_sv, "d"_sv });

    ASSERT_EQ(di::at(m, "a"_sv), "b"_sv);
    ASSERT_EQ(di::at_unchecked(m, "a"_sv), "b"_sv);
}

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
    auto c = std::initializer_list<int> { 0, 1, 2 };
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

    auto g = di::range(6);
    auto r1 = di::fold_right(g, 0, di::plus);
    ASSERT_EQ(r1, 15);

    auto r2 = di::fold_right_last(g, di::plus);
    ASSERT_EQ(*r2, 15);

    auto r3 = di::fold_left_first(g, di::plus);
    ASSERT_EQ(*r3, 15);
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

    auto c = di::Array { 5, 5, 6, 6, 6, 7, 7 } | di::to<di::Vector>();
    {
        auto [s, e] = di::unique(c);
        c.erase(s, e);
    }

    auto ex1 = di::Array { 5, 6, 7 } | di::to<di::Vector>();
    ASSERT_EQ(c, ex1);

    auto d = di::Array { 5, 5 } | di::to<di::Vector>();
    {
        auto [s, e] = di::unique(d);
        d.erase(s, e);
    }

    auto ex2 = di::Array { 5 } | di::to<di::Vector>();
    ASSERT_EQ(d, ex2);
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

    auto d = di::Array { 1, 1, 2, 2, 2, 3 };
    auto r1 = di::search_n(d, 3, 2);
    ASSERT(r1);
    ASSERT_EQ(r1.begin(), d.begin() + 2);
    ASSERT_EQ(r1.end(), d.begin() + 5);
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

constexpr void shift() {
    auto x = di::Array { 1, 2, 3, 4, 5 };

    auto r1 = di::shift_left(x, 2);
    ASSERT_EQ(r1.begin(), x.begin());
    ASSERT_EQ(r1.end(), x.begin() + 3);

    auto ex1 = di::Array { 3, 4, 5, 0, 0 };
    di::fill(r1.end(), x.end(), 0);
    ASSERT_EQ(x, ex1);

    auto r2 = di::shift_left(x, 0);
    ASSERT_EQ(r2.begin(), x.begin());
    ASSERT_EQ(r2.end(), x.end());

    auto ex2 = di::Array { 3, 4, 5, 0, 0 };
    ASSERT_EQ(x, ex2);

    auto y = di::Array { 1, 2, 3, 4, 5 };

    auto r3 = di::shift_right(y, 2);
    ASSERT_EQ(r3.begin(), y.begin() + 2);
    ASSERT_EQ(r3.end(), y.end());

    auto ex3 = di::Array { 0, 0, 1, 2, 3 };
    di::fill(y.begin(), r3.begin(), 0);
    ASSERT_EQ(y, ex3);

    auto r4 = di::shift_right(y, 0);
    ASSERT_EQ(r4.begin(), y.begin());
    ASSERT_EQ(r4.end(), y.end());

    auto ex4 = di::Array { 0, 0, 1, 2, 3 };
    ASSERT_EQ(y, ex4);

    auto z = di::Array { 1, 2, 3, 4, 5 };

    auto r5 = di::shift_right(z, 4);
    ASSERT_EQ(r5.begin(), z.begin() + 4);
    ASSERT_EQ(r5.end(), z.end());

    auto ex5 = di::Array { 0, 0, 0, 0, 1 };
    di::fill(z.begin(), r5.begin(), 0);
    ASSERT_EQ(z, ex5);

    auto q = di::Array { 1, 2, 3, 4, 5 };

    auto r6 = di::shift_left(q, 4);
    ASSERT_EQ(r6.begin(), q.begin());
    ASSERT_EQ(r6.end(), q.begin() + 1);

    auto ex6 = di::Array { 5, 0, 0, 0, 0 };
    di::fill(r6.end(), q.end(), 0);
    ASSERT_EQ(q, ex6);

    // FIXME: add tests for shift_right with forward containers
    //        using di::ForwardList.
}

constexpr void partition() {
    auto a = di::Array { 2, 1, 3, 4, 6, 5 };

    auto r1 = di::partition(a, [](int x) {
        return x % 2 == 1;
    });
    ASSERT_EQ(r1.begin(), a.begin() + 3);
    ASSERT_EQ(r1.end(), a.end());
    ASSERT(di::is_partitioned(a, [](int x) {
        return x % 2 == 1;
    }));
    ASSERT(!di::is_partitioned(a, [](int x) {
        return x % 3 == 0;
    }));

    a = di::Array { 1, 2, 3, 4, 5, 6 };
    auto b = di::Array<int, 3> {};
    auto c = di::Array<int, 3> {};
    auto r2 = di::partition_copy(a, b.begin(), c.begin(), [](int x) {
        return x % 2 == 0;
    });

    ASSERT_EQ(r2.in, a.end());
    ASSERT_EQ(r2.out1, b.end());
    ASSERT_EQ(r2.out2, c.end());

    auto ex1 = di::Array { 2, 4, 6 };
    auto ex2 = di::Array { 1, 3, 5 };
    ASSERT_EQ(b, ex1);
    ASSERT_EQ(c, ex2);

    auto d = di::Array { 7, 1, 4, 2, 3, 5, 6 };
    auto r3 = di::stable_partition(d, [](int x) {
        return x % 2 == 0;
    });
    ASSERT_EQ(r3.begin(), d.begin() + 3);
    ASSERT_EQ(r3.end(), d.end());

    auto ex3 = di::Array { 4, 2, 6, 7, 1, 3, 5 };
    ASSERT_EQ(d, ex3);

    auto r4 = di::partition_point(d, [](int x) {
        return x % 2 == 0;
    });
    ASSERT_EQ(r4, d.begin() + 3);
}

constexpr void permutation() {
    auto a = di::Array { 1, 5, 1, 2, 3 };
    auto b = di::Array { 5, 3, 1, 1, 2 };
    auto c = di::Array { 5, 1, 3, 1, 1 };
    auto d = di::Array { 5, 1, 3, 1, 1, 2 };

    ASSERT(di::is_permutation(a, b));
    ASSERT(!di::is_permutation(a, c));
    ASSERT(!di::is_permutation(a, d));

    auto e = di::Array { 1, 2, 3 };
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 1, 3, 2 }));
    }
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 2, 1, 3 }));
    }
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 2, 3, 1 }));
    }
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 3, 1, 2 }));
    }
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 3, 2, 1 }));
    }
    {
        auto r1 = di::next_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, false);
        ASSERT_EQ(e, (di::Array { 1, 2, 3 }));
    }

    e = di::Array { 3, 2, 1 };
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 3, 1, 2 }));
    }
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 2, 3, 1 }));
    }
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 2, 1, 3 }));
    }
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 1, 3, 2 }));
    }
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(e, (di::Array { 1, 2, 3 }));
    }
    {
        auto r1 = di::prev_permutation(e);
        ASSERT_EQ(r1.in, e.end());
        ASSERT_EQ(r1.found, false);
        ASSERT_EQ(e, (di::Array { 3, 2, 1 }));
    }

    auto f = di::Array { 1, 1, 2, 2 };
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(f, (di::Array { 1, 2, 1, 2 }));
    }
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(f, (di::Array { 1, 2, 2, 1 }));
    }
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(f, (di::Array { 2, 1, 1, 2 }));
    }
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(f, (di::Array { 2, 1, 2, 1 }));
    }
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, true);
        ASSERT_EQ(f, (di::Array { 2, 2, 1, 1 }));
    }
    {
        auto r1 = di::next_permutation(f);
        ASSERT_EQ(r1.in, f.end());
        ASSERT_EQ(r1.found, false);
        ASSERT_EQ(f, (di::Array { 1, 1, 2, 2 }));
    }
}

constexpr void binary_search() {
    auto a = di::Array { 1, 2, 3, 3, 3, 4, 6 };
    for (auto& x : a) {
        auto [it, found] = di::binary_search(a, x);
        ASSERT(found);
        ASSERT_EQ(*it, x);
    }

    auto b = di::Array { 1, 2, 3, 3, 3, 4, 6, 6 };
    for (auto& x : b) {
        auto [it, found] = di::binary_search(b, x);
        ASSERT(found);
        ASSERT_EQ(*it, x);
    }

    for (auto& a : di::Array<di::Span<int>, 2> { a.span(), b.span() }) {
        {
            auto [it, found] = di::binary_search(a, 0);
            ASSERT_EQ(it, a.begin());
            ASSERT(!found);
        }
        {
            auto [it, found] = di::binary_search(a, 5);
            ASSERT_EQ(it, a.begin() + 6);
            ASSERT(!found);
        }
        {
            auto [it, found] = di::binary_search(a, 7);
            ASSERT_EQ(it, a.end());
            ASSERT(!found);
        }
    }

    {
        auto it = di::lower_bound(a, 3);
        ASSERT_EQ(it, a.begin() + 2);
    }
    {
        auto it = di::upper_bound(a, 3);
        ASSERT_EQ(it, a.begin() + 5);
    }
    {
        auto [it, jt] = di::equal_range(a, 3);
        ASSERT_EQ(it, a.begin() + 2);
        ASSERT_EQ(jt, a.begin() + 5);
    }
}

constexpr void set() {
    auto a = di::Array { 1, 1, 2, 4, 5, 6 };
    auto b = di::Array { 1, 2, 3, 4, 5 };

    auto r1 = di::Array<int, a.size() + b.size()> {};
    auto e1 = di::Array { 1, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6 };

    di::merge(a, b, r1.begin());
    ASSERT_EQ(r1, e1);

    auto r2 = di::Array<int, 2> {};
    auto e2 = di::Array { 1, 6 };

    di::set_difference(a, b, r2.begin());
    ASSERT_EQ(r2, e2);

    auto r3 = di::Array<int, 4> {};
    auto e3 = di::Array { 1, 2, 4, 5 };

    di::set_intersection(a, b, r3.begin());
    ASSERT_EQ(r3, e3);

    auto r4 = di::Array<int, 3> {};
    auto e4 = di::Array { 1, 3, 6 };

    di::set_symmetric_difference(a, b, r4.begin());
    ASSERT_EQ(r4, e4);

    auto c = di::Array { 1, 1, 4 };
    auto d = di::Array { 1, 1, 1, 4 };

    ASSERT(di::container::includes(a, c));
    ASSERT(!di::container::includes(a, d));
}

TESTC(container_algorithm, access)
TESTC(container_algorithm, minmax)
TESTC(container_algorithm, compare)
TESTC(container_algorithm, fold)
TESTC(container_algorithm, is_sorted)
TESTC(container_algorithm, permute)
TESTC(container_algorithm, contains)
TESTC(container_algorithm, predicate)
TESTC(container_algorithm, for_each)
TESTC(container_algorithm, sort)
TESTC(container_algorithm, shift)
TESTC(container_algorithm, partition)
TESTC(container_algorithm, permutation)
TESTC(container_algorithm, binary_search)
TESTC(container_algorithm, set)
}
