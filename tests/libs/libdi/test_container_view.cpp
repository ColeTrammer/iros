#include <di/container/concepts/random_access_container.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/view/all.h>
#include <di/container/view/empty.h>
#include <di/container/view/iota.h>
#include <di/container/view/owning_view.h>
#include <di/container/view/range.h>
#include <di/container/view/ref_view.h>
#include <di/container/view/single.h>
#include <di/container/view/view.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    int arr[] = { 1, 2, 3, 4, 5 };
    auto x = di::container::View { di::container::begin(arr), di::container::end(arr) };

    auto [s, e] = x;
    EXPECT_EQ(s, arr + 0);
    EXPECT_EQ(e, arr + 5);

    {
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }
    {
        x.advance(2);
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        EXPECT_EQ(sum, 12);
    }
}

constexpr void all() {
    int arr[] = { 1, 2, 3, 4, 5 };
    auto x = di::container::view::all(arr);

    {
        static_assert(di::concepts::BorrowedContainer<decltype(x)>);
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::container::view::all(x)) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }

    {
        struct X {
            constexpr int* begin() { return arr + 0; }
            constexpr int* end() { return arr + 5; }

            int arr[5];
        };

        auto sum = 0;
        auto v = di::container::view::all(X { 1, 2, 3, 4, 5 });
        static_assert(!di::concepts::BorrowedContainer<decltype(v)>);
        for (auto z : v) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : arr | di::container::view::all) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : arr | (di::container::view::all | di::container::view::all)) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }
}

constexpr void empty() {
    auto c = di::container::view::empty<int>;
    for (auto x : c) {
        (void) x;
        EXPECT(false);
    }
    EXPECT(c.empty());
}

constexpr void single() {
    auto c = di::container::view::single(5);

    {
        auto sum = 0;
        for (auto z : c | di::container::view::all) {
            sum += z;
        }
        EXPECT_EQ(sum, 5);
    }

    EXPECT_EQ(c.size(), 1u);
}

constexpr void iota() {
    static_assert(di::concepts::Iterator<decltype(di::container::view::iota(1, 6).begin())>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::container::view::iota(1, 6))>);
    static_assert(di::concepts::RandomAccessContainer<decltype(di::container::view::iota(1))>);
    static_assert(!di::concepts::CommonContainer<decltype(di::container::view::iota(1))>);

    {
        auto sum = 0;
        for (auto z : di::container::view::iota(1, 6)) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }

    {
        auto sum = 0;
        for (auto z : di::container::view::range(6)) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }
}

TEST_CONSTEXPR(container_view, basic, basic)
TEST_CONSTEXPR(container_view, all, all)
TEST_CONSTEXPR(container_view, empty, empty)
TEST_CONSTEXPR(container_view, single, single)
TEST_CONSTEXPR(container_view, iota, iota)
