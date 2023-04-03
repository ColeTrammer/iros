#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace container_view {
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

constexpr void adjacent_transform() {
    int arr[] = { 1, 2, 3, 4, 5, 6 };

    static_assert(di::concepts::RandomAccessContainer<decltype(di::pairwise_transform(arr, di::plus))>);

    for (auto c : arr | di::pairwise_transform(di::less)) {
        ASSERT(c);
    }

    auto windows = arr | di::adjacent_transform<3>([](auto x, auto y, auto z) {
                       return x + y + z;
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
    ASSERT(
        di::container::equal(di::range(10) | di::take_while(di::curry_back(di::less)(5)), di::Array { 0, 1, 2, 3, 4 }));
}

constexpr void drop_while() {
    ASSERT(
        di::container::equal(di::range(10) | di::drop_while(di::curry_back(di::less)(5)), di::Array { 5, 6, 7, 8, 9 }));
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

constexpr void chunk_by() {
    auto in1 = di::Array { 1, 2, 2, 3, 0, 4, 5, 2 };
    auto r1 = di::chunk_by(in1, di::equal_or_less) | di::to<di::Vector>();
    auto ex1 = di::Array { *in1.subspan(0, 4), *in1.subspan(4, 3), *in1.subspan(7) } | di::to<di::Vector>();
    ASSERT_EQ(r1, ex1);

    auto r2 = di::chunk_by(in1, di::equal_or_less) | di::reverse | di::to<di::Vector>();
    auto ex2 =
        di::Array { *in1.subspan(0, 4), *in1.subspan(4, 3), *in1.subspan(7) } | di::reverse | di::to<di::Vector>();
    ASSERT_EQ(r2, ex2);
}

constexpr void cartesian_product() {
    auto r1 = di::cartesian_product(di::range(2), di::range(2), di::range(2)) | di::to<di::Vector>();
    auto ex1 = di::Array {
        di::Tuple { 0, 0, 0 }, di::Tuple { 0, 0, 1 }, di::Tuple { 0, 1, 0 }, di::Tuple { 0, 1, 1 },
        di::Tuple { 1, 0, 0 }, di::Tuple { 1, 0, 1 }, di::Tuple { 1, 1, 0 }, di::Tuple { 1, 1, 1 },
    } | di::to<di::Vector>();

    ASSERT_EQ(r1, ex1);

    auto r2 = di::cartesian_product(di::range(2), di::range(3), di::range(4), di::range(5));
    auto ex2 = r2 | di::to<di::Vector>();

    ASSERT_EQ(r2.size(), 2u * 3u * 4u * 5u);

    auto b = r2.begin();
    auto e = r2.end();

    ASSERT_EQ(b + 120, e);
    for (ssize_t i = 0; i < di::ssize(r2); i++) {
        ASSERT_EQ(b[i], ex2[i]);
        ASSERT_EQ(e[-(di::ssize(r2) - i)], ex2[i]);
        ASSERT_EQ((b + i) - b, i);
        ASSERT_EQ((e - (di::ssize(r2) - i)) - b, i);
    }

    auto r3 = di::cartesian_product(di::range(5), di::view::empty<int>);
    ASSERT_EQ(r3.begin(), r3.end());
}

constexpr void common() {
    auto in1 = di::range(1, 4) | di::cycle | di::take(9) | di::common;
    static_assert(di::concepts::CommonContainer<decltype(in1)>);

    auto r3 = di::move(in1) | di::chunk(3) | di::transform(di::sum) | di::to<di::Vector>();
    auto ex3 = di::Array { 6, 6, 6 } | di::to<di::Vector>();
    ASSERT_EQ(r3, ex3);
}

constexpr void concat() {
    auto in1 = di::Array { 2, 3, 4 };
    auto in2 = di::Array { 7, 6, 5 };
    auto in3 = di::Array { 1, 8, 0 };

    auto v1 = di::concat(in1, in2, in3);
    static_assert(di::concepts::View<decltype(v1)>);
    static_assert(di::concepts::RandomAccessContainer<decltype(v1)>);
    static_assert(di::concepts::SizedContainer<decltype(v1)>);
    static_assert(di::SameAs<int&, di::meta::ContainerReference<decltype(v1)>>);
    static_assert(di::concepts::Permutable<decltype(v1.begin())>);

    auto r1 = di::move(v1) | di::to<di::Vector>();
    auto ex1 = di::Array { 2, 3, 4, 7, 6, 5, 1, 8, 0 } | di::to<di::Vector>();
    ASSERT_EQ(r1, ex1);

    ASSERT_EQ(v1.size(), 9u);

    ASSERT_EQ((v1.begin() + 6) - (v1.begin() + 1), 5);
    ASSERT_EQ((v1.begin() + 7) - (v1.begin() + 2), 5);
    ASSERT_EQ((v1.begin() + 7) - (v1.begin() + 4), 3);
    ASSERT_EQ((v1.begin() + 7) - (di::container::default_sentinel), -2);
    ASSERT_EQ((v1.begin() + 6) - (di::container::default_sentinel), -3);
    ASSERT_EQ((v1.begin() + 5) - (di::container::default_sentinel), -4);

    di::sort(v1);
    ASSERT_EQ(in1, di::to_array({ 0, 1, 2 }));
    ASSERT_EQ(in2, di::to_array({ 3, 4, 5 }));
    ASSERT_EQ(in3, di::to_array({ 6, 7, 8 }));

    auto x = di::Array { 4, 4, 4 } | di::to<di::Vector>();
    auto v2 = di::concat(di::Optional { 5 }, di::move(x), di::Optional { 3 });

    static_assert(di::SameAs<int&, di::meta::ContainerReference<decltype(v2)>>);

    auto r2 = di::move(v2) | di::to<di::Vector>();
    auto ex2 = di::Array { 5, 4, 4, 4, 3 } | di::to<di::Vector>();

    ASSERT_EQ(r2, ex2);
}

TESTC(container_view, basic)
TESTC(container_view, all)
TESTC(container_view, empty)
TESTC(container_view, single)
TESTC(container_view, iota)
TESTC(container_view, repeat)
TESTC(container_view, reverse)
TESTC(container_view, as_rvalue)
TESTC(container_view, as_const)
TESTC(container_view, transform)
TESTC(container_view, zip)
TESTC(container_view, zip_transform)
TESTC(container_view, adjacent)
TESTC(container_view, adjacent_transform)
TESTC(container_view, counted)
TESTC(container_view, take)
TESTC(container_view, drop)
TESTC(container_view, split)
TESTC(container_view, join)
TESTC(container_view, join_with)
TESTC(container_view, filter)
TESTC(container_view, take_while)
TESTC(container_view, drop_while)
TESTC(container_view, elements)
TESTC(container_view, stride)
TESTC(container_view, enumerate)
TESTC(container_view, cycle)
TESTC(container_view, chunk)
TEST(container_view, chunk_generator)
TESTC(container_view, slide)
TESTC(container_view, chunk_by)
TESTC(container_view, cartesian_product)
TESTC(container_view, common)
TESTC(container_view, concat)
}
