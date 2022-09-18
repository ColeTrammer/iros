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

    static_assert(di::concepts::View<decltype(di::transform(di::view::all(arr), di::id))>);
    static_assert(di::concepts::View<decltype(arr | di::transform(di::id))>);
    static_assert(di::concepts::View<decltype(di::transform(di::id)(arr))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::transform(di::id)(arr))>);
    static_assert(!di::concepts::ContiguousContainer<decltype(di::transform(di::id)(arr))>);

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

TEST_CONSTEXPR(container_view, basic, basic)
TEST_CONSTEXPR(container_view, all, all)
TEST_CONSTEXPR(container_view, empty, empty)
TEST_CONSTEXPR(container_view, single, single)
TEST_CONSTEXPR(container_view, iota, iota)
TEST_CONSTEXPR(container_view, repeat, repeat)
TEST_CONSTEXPR(container_view, reverse, reverse)
TEST_CONSTEXPR(container_view, as_rvalue, as_rvalue)
TEST_CONSTEXPR(container_view, transform, transform)
