#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    int arr[] = { 1, 2, 3, 4, 5 };
    auto x = di::View { di::begin(arr), di::end(arr) };

    auto [s, e] = x;
    ASSERT_EQ(s, arr + 0);
    ASSERT_EQ(e, arr + 5);

    {
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }
    {
        x.advance(2);
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        ASSERT_EQ(sum, 12);
    }
}

constexpr void all() {
    int arr[] = { 1, 2, 3, 4, 5 };
    auto x = di::view::all(arr);

    {
        static_assert(di::concepts::BorrowedContainer<decltype(x)>);
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::view::all(x)) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        struct X {
            constexpr int* begin() { return arr + 0; }
            constexpr int* end() { return arr + 5; }

            int arr[5];
        };

        auto sum = 0;
        auto v = di::view::all(X { 1, 2, 3, 4, 5 });
        static_assert(!di::concepts::BorrowedContainer<decltype(v)>);
        for (auto z : v) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : arr | di::view::all) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : arr | (di::view::all | di::view::all)) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }
}

constexpr void empty() {
    auto c = di::view::empty<int>;
    for (auto x : c) {
        (void) x;
        ASSERT(false);
    }
    ASSERT(c.empty());
}

constexpr void single() {
    auto c = di::single(5);

    {
        auto sum = 0;
        for (auto z : c | di::view::all) {
            sum += z;
        }
        ASSERT_EQ(sum, 5);
    }

    ASSERT_EQ(c.size(), 1u);
}

constexpr void iota() {
    static_assert(di::concepts::Iterator<decltype(di::view::iota(1, 6).begin())>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::view::iota(1, 6))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::view::iota(1))>);
    static_assert(!di::concepts::CommonContainer<decltype(di::view::iota(1))>);

    {
        auto sum = 0;
        for (auto z : di::view::iota(1, 6)) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::range(6)) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }
}

constexpr void repeat() {
    static_assert(di::concepts::RandomAccessContainer<decltype(di::repeat(5, 5))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::repeat(5))>);

    {
        auto sum = 0;
        for (auto z : di::repeat(5, 5)) {
            sum += z;
        }
        ASSERT_EQ(sum, 25);
    }
}

constexpr void reverse() {
    int arr[] = { 1, 2, 3, 4, 5 };
    static_assert(!di::concepts::ContiguousIterator<di::container::ReverseIterator<int*>>);
    static_assert(di::concepts::RandomAccessIterator<di::container::ReverseIterator<int*>>);
    static_assert(di::concepts::BidirectionalContainer<decltype(di::reverse(arr))>);
    static_assert(di::concepts::SizedContainer<decltype(di::reverse(arr))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::reverse(arr))>);
    static_assert(!di::concepts::ContiguousContainer<decltype(di::reverse(arr))>);

    static_assert(di::SameAs<decltype(di::range(6)), decltype(di::range(6) | di::reverse | di::reverse)>);

    using V = di::container::View<di::container::ReverseIterator<int*>, di::container::ReverseIterator<int*>, true>;
    static_assert(di::SameAs<di::container::View<int*, int*, true>, decltype(di::declval<V>() | di::reverse)>);

    {
        auto v = arr | di::reverse;
        ASSERT_EQ(v[0], 5);
        ASSERT_EQ(v[1], 4);
        ASSERT_EQ(v[2], 3);
    }

    {
        auto sum = 0;
        for (auto z : di::range(6) | di::reverse) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::range(6) | di::reverse | di::reverse) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }
}

constexpr void as_const() {
    int arr[] = { 1, 2, 3, 4, 5 };

    static_assert(di::concepts::BidirectionalContainer<decltype(di::view::as_const(arr))>);
    static_assert(di::concepts::SizedContainer<decltype(di::view::as_const(arr))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::view::as_const(arr))>);
    static_assert(di::SameAs<int const&, di::meta::ContainerReference<decltype(di::view::as_const(arr))>>);
    static_assert(di::concepts::ContiguousContainer<decltype(di::view::as_const(arr))>);

    ASSERT_EQ(di::sum(di::view::as_const(arr)), 15);
}

constexpr void as_rvalue() {
    int arr[] = { 1, 2, 3, 4, 5 };
    static_assert(!di::concepts::ContiguousIterator<di::container::MoveIterator<int*>>);
    static_assert(di::concepts::RandomAccessIterator<di::container::MoveIterator<int*>>);
    static_assert(di::concepts::BidirectionalContainer<decltype(di::as_rvalue(arr))>);
    static_assert(di::concepts::SizedContainer<decltype(di::as_rvalue(arr))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::as_rvalue(arr))>);
    static_assert(!di::concepts::ContiguousContainer<decltype(di::as_rvalue(arr))>);

    static_assert(di::SameAs<di::meta::ContainerReference<decltype(di::as_rvalue(arr))>, int&&>);

    {
        auto sum = 0;
        for (auto z : arr | di::as_rvalue) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::range(6) | di::as_rvalue) {
            sum += z;
        }
        ASSERT_EQ(sum, 15);
    }
}

constexpr void transform() {
    int arr[] = { 0, 1, 2, 3, 4 };

    static_assert(di::concepts::View<decltype(di::transform(di::view::all(arr), di::identity))>);
    static_assert(di::concepts::View<decltype(arr | di::transform(di::identity))>);
    static_assert(di::concepts::View<decltype(di::transform(di::identity)(arr))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::transform(di::identity)(arr))>);
    static_assert(!di::concepts::ContiguousContainer<decltype(di::transform(di::identity)(arr))>);

    {
        int sum = 0;
        for (auto x : arr | di::transform([](auto x) {
                          return x + 1;
                      })) {
            sum += x;
        }
        ASSERT_EQ(sum, 15);
    }
}

constexpr void zip() {
    int arr[] = { 0, 1, 2, 3, 4 };
    int ar[] = { 4, 3, 2, 1, 0 };

    for (auto [a, b] : di::zip(arr, ar)) {
        ASSERT_EQ(a + b, 4);
    }

    for (auto [a, b, c, d] : di::zip(di::range(5), arr, di::reverse(ar), di::range(100))) {
        ASSERT_EQ(a, b);
        ASSERT_EQ(a, c);
        ASSERT_EQ(a, d);
    }

    static_assert(di::concepts::RandomAccessContainer<decltype(di::zip(arr, ar))>);
}

constexpr void zip_transform() {
    int arr[] = { 0, 1, 2, 3, 4 };
    int ar[] = { 4, 3, 2, 1, 0 };

    for (auto c : di::zip_transform(di::plus, arr, ar)) {
        ASSERT_EQ(c, 4);
    }

    for (auto e : di::zip_transform(
             [](auto w, auto x, auto y, auto z) {
                 return w == x && y == z && x == y;
             },
             di::range(5), arr, di::reverse(ar), di::range(100))) {
        ASSERT(e);
    }

    static_assert(di::concepts::RandomAccessContainer<decltype(di::zip_transform(di::plus, arr, ar))>);
}

constexpr void adjacent() {
    int arr[] = { 1, 2, 3, 4, 5, 6 };

    static_assert(di::concepts::RandomAccessContainer<decltype(di::pairwise(arr))>);

    for (auto [a, b] : arr | di::pairwise) {
        ASSERT_EQ(a + 1, b);
    }

    auto windows = arr | di::adjacent<3> | di::transform([](auto x) {
                       auto [a, b, c] = x;
                       return a + b + c;
                   });
    ASSERT(di::container::equal(windows, di::Array { 6, 9, 12, 15 }));
}

constexpr void counted() {
    auto x = di::Array { 5, 4, 3, 2, 1 };
    auto y = di::view::counted(x.begin(), 3);
    auto r = di::range(3, 8);
    auto z = di::reverse(di::view::counted(r.begin(), 3));
    ASSERT(di::container::equal(y, di::Array { 5, 4, 3 }));
    ASSERT(di::container::equal(z, di::Array { 5, 4, 3 }));
}

constexpr void take() {
    auto a = di::Array { 1, 2, 3, 4, 5 };
    auto x = a | di::take(3);
    static_assert(di::SameAs<decltype(x), di::Span<int>>);
    ASSERT(di::container::equal(x, di::Array { 1, 2, 3 }));

    auto z = di::iota(1) | di::take(3);
    ASSERT(di::container::equal(z, di::Array { 1, 2, 3 }));
}

constexpr void drop() {
    auto a = di::Array { 1, 2, 3, 4, 5 };
    auto x = a | di::drop(2);
    static_assert(di::SameAs<decltype(x), di::Span<int>>);
    ASSERT(di::container::equal(x, di::Array { 3, 4, 5 }));

    auto z = di::iota(1) | di::drop(2) | di::take(3);
    ASSERT(di::container::equal(z, di::Array { 3, 4, 5 }));
}

constexpr void split() {
    auto a = di::Array { 1, 2, 4, 3, 5, 4, 1, 2, 4, 4, 4, 4 } | di::split(4);
    auto b = di::move(a) | di::transform(di::sum);

    static_assert(di::SameAs<decltype(*a.begin()), di::Span<int>>);

    ASSERT(di::container::equal(b, di::Array { 3, 8, 3, 0, 0, 0, 0 }));

    auto c = u8"Hello, world, friends"_sv;
    auto d = c | di::split(u8", "_sv);

    static_assert(di::SameAs<decltype(*d.begin()), decltype(u8"Hello"_sv)>);

    ASSERT(di::container::equal(d, di::Array { u8"Hello"_sv, u8"world"_sv, u8"friends"_sv }));
}

constexpr void join() {
    auto a = di::Array { di::Array { 1, 2 }, di::Array { 3, 4 } };

    ASSERT(di::container::equal(a | di::join, di::Array { 1, 2, 3, 4 }));

    auto b = di::range(2) | di::transform([](auto) {
                 return di::Array { 1, 2 };
             });
    ASSERT(di::container::equal(b | di::join, di::Array { 1, 2, 1, 2 }));
}

constexpr void join_with() {
    auto a = di::Array { di::Array { 1, 2 }, di::Array { 3, 4 } };

    ASSERT(di::container::equal(a | di::join_with(5), di::Array { 1, 2, 5, 3, 4 }));

    auto b = di::range(2) | di::transform([](auto) {
                 return di::Array { 1, 2 };
             });
    ASSERT(di::container::equal(b | di::join_with(di::Array { 3, 4 }), di::Array { 1, 2, 3, 4, 1, 2 }));
}

constexpr void filter() {
    auto x = di::Array { 1, 2, 3, 4, 5 };
    auto y = di::filter(x, [](auto x) {
        return x % 2 == 1;
    });

    static_assert(di::concepts::Container<decltype(y)>);
    ASSERT_EQ(di::container::distance(y), 3);
    ASSERT_EQ(*y.front(), 1);

    ASSERT(di::container::equal(x | di::filter([](auto x) {
                                    return x % 2 == 1;
                                }),
                                di::Array { 1, 3, 5 }));
    ASSERT(di::container::equal(x | di::reverse | di::filter([](auto x) {
                                    return x % 2 == 1;
                                }),
                                di::Array { 5, 3, 1 }));
    ASSERT(di::container::equal(x | di::filter([](auto x) {
                                    return x % 2 == 1;
                                }) | di::reverse,
                                di::Array { 5, 3, 1 }));
}

constexpr void take_while() {
    ASSERT(di::container::equal(di::range(10) | di::take_while(di::curry_back(di::less)(5)), di::Array { 0, 1, 2, 3, 4 }));
}

constexpr void drop_while() {
    ASSERT(di::container::equal(di::range(10) | di::drop_while(di::curry_back(di::less)(5)), di::Array { 5, 6, 7, 8, 9 }));
}

constexpr void elements() {
    auto x = di::zip(di::range(5), di::range(5));
    auto y = x | di::to<di::Vector>();
    ASSERT(di::container::equal(di::keys(x), di::values(x)));
    ASSERT(di::container::equal(di::keys(y), di::values(y)));
    ASSERT(di::container::equal(di::keys(y) | di::reverse, di::values(y) | di::reverse));
}

constexpr void stride() {
    auto x = di::range(5) | di::stride(2);
    auto y = di::range(5) | di::stride(3);
    auto z = di::range(5) | di::stride(1);
    ASSERT(di::container::equal(x, di::Array { 0, 2, 4 }));
    ASSERT(di::container::equal(y, di::Array { 0, 3 }));
    ASSERT(di::container::equal(z, di::Array { 0, 1, 2, 3, 4 }));
}

constexpr void enumerate() {
    ASSERT(di::all_of(di::range(10) | di::enumerate, [](auto pair) {
        auto [index, value] = pair;
        return index == di::to_unsigned(value);
    }));
}

constexpr void cycle() {
    auto x = di::Array { 1, 2, 3, 4, 5 } | di::cycle;

    auto y = di::move(x) | di::take(15) | di::to<di::Vector>();
    auto ex1 = di::Array { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 } | di::to<di::Vector>();
    ASSERT_EQ(y, ex1);

    auto z = di::move(x) | di::take(15) | di::reverse | di::to<di::Vector>();
    auto ex2 = di::Array { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 } | di::reverse | di::to<di::Vector>();
    ASSERT_EQ(z, ex2);

    auto a = x.begin();
    auto b = a + 50;

    ASSERT_EQ(a[20], 1);
    ASSERT_EQ(di::distance(a, b), 50);
}

constexpr void chunk() {
    auto in1 = di::Array { 1, 2, 3, 8, 2, -3, 9, -1 };

    auto r1 = in1 | di::chunk(3) | di::transform(di::sum) | di::to<di::Vector>();
    auto ex1 = di::Array { 6, 7, 8 } | di::to<di::Vector>();
    ASSERT_EQ(r1, ex1);

    auto r2 = in1 | di::chunk(3) | di::reverse | di::transform(di::sum) | di::to<di::Vector>();
    auto ex2 = di::Array { 8, 7, 6 } | di::to<di::Vector>();
    ASSERT_EQ(r2, ex2);

    ASSERT_EQ(di::size(in1 | di::chunk(3)), 3u);
}

void chunk_generator() {
    auto in3 = []() -> di::Generator<int> {
        co_yield 1;
        co_yield 2;
        co_yield 3;
        co_yield 8;
        co_yield 2;
        co_yield -3;
        co_yield 9;
        co_yield -1;
    }();

    auto r3 = di::move(in3) | di::chunk(3) | di::transform(di::sum) | di::to<di::Vector>();
    auto ex3 = di::Array { 6, 7, 8 } | di::to<di::Vector>();
    ASSERT_EQ(r3, ex3);
}

constexpr void slide() {
    auto in1 = di::Array { 1, 2, 3, 4, 5 };
    auto r1 = in1 | di::slide(3) | di::transform(di::sum) | di::to<di::Vector>();
    auto ex1 = di::Array { 6, 9, 12 } | di::to<di::Vector>();
    ASSERT_EQ(r1, ex1);

    auto in2 = "abcde"_sv;
    auto r2 = in2 | di::slide(3) | di::to<di::Vector>();
    auto ex2 = di::Array { "abc"_sv, "bcd"_sv, "cde"_sv } | di::to<di::Vector>();
    ASSERT_EQ(r2, ex2);

    auto in3 = "abcde"_sv;
    auto r3 = in3 | di::slide(3) | di::reverse | di::to<di::Vector>();
    auto ex3 = di::Array { "abc"_sv, "bcd"_sv, "cde"_sv } | di::reverse | di::to<di::Vector>();
    ASSERT_EQ(r3, ex3);
}

TEST_CONSTEXPR(container_view, basic, basic)
TEST_CONSTEXPR(container_view, all, all)
TEST_CONSTEXPR(container_view, empty, empty)
TEST_CONSTEXPR(container_view, single, single)
TEST_CONSTEXPR(container_view, iota, iota)
TEST_CONSTEXPR(container_view, repeat, repeat)
TEST_CONSTEXPR(container_view, reverse, reverse)
TEST_CONSTEXPR(container_view, as_rvalue, as_rvalue)
TEST_CONSTEXPR(container_view, as_const, as_const)
TEST_CONSTEXPR(container_view, transform, transform)
TEST_CONSTEXPR(container_view, zip, zip)
TEST_CONSTEXPR(container_view, zip_transform, zip_transform)
TEST_CONSTEXPR(container_view, adjacent, adjacent)
TEST_CONSTEXPR(container_view, counted, counted)
TEST_CONSTEXPR(container_view, take, take)
TEST_CONSTEXPR(container_view, drop, drop)
TEST_CONSTEXPR(container_view, split, split)
TEST_CONSTEXPR(container_view, join, join)
TEST_CONSTEXPR(container_view, join_with, join_with)
TEST_CONSTEXPR(container_view, filter, filter)
TEST_CONSTEXPR(container_view, take_while, take_while)
TEST_CONSTEXPR(container_view, drop_while, drop_while)
TEST_CONSTEXPR(container_view, elements, elements)
TEST_CONSTEXPR(container_view, stride, stride)
TEST_CONSTEXPR(container_view, enumerate, enumerate)
TEST_CONSTEXPR(container_view, cycle, cycle)
TEST_CONSTEXPR(container_view, chunk, chunk)
TEST_CONSTEXPRX(container_view, chunk_generator, chunk_generator)
TEST_CONSTEXPR(container_view, slide, slide)
