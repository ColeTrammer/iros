#include <di/util/prelude.h>
#include <dius/test/prelude.h>

namespace util_swap {
constexpr static void basic() {
    static_assert(di::concepts::Swappable<int>);
    static_assert(!di::concepts::SwappableWith<int, long>);
    static_assert(!di::concepts::SwappableWith<char, char const*>);

    int a = 13;
    int b = 26;
    di::swap(a, b);
    ASSERT_EQ(a, 26);
    ASSERT_EQ(b, 13);
}

struct X {
    int* p;

private:
    constexpr friend void tag_invoke(di::Tag<di::swap>, X& a, X& b) {
        di::swap(a.p, b.p);
        ++(*a.p);
    }
};

constexpr static void custom() {
    static_assert(di::concepts::Swappable<X>);

    int c = 0;

    auto a = X { &c };
    auto b = X { &c };
    di::swap(a, b);

    ASSERT_EQ(c, 1);
}

struct Y {
    int x;

    constexpr Y(int v) : x(v) {}
    constexpr Y(Y&&) = default;
    constexpr auto operator=(Y&&) -> Y& = delete;
};

constexpr static void non_assignable() {
    static_assert(!di::concepts::Swappable<Y>);
    static_assert(di::concepts::Swappable<di::util::RebindableBox<Y>>);

    auto a = di::util::RebindableBox { Y { 3 } };
    auto b = di::util::RebindableBox { Y { 7 } };
    di::swap(a, b);

    ASSERT_EQ(a.value().x, 7);
    ASSERT_EQ(b.value().x, 3);
}

struct Z {
    int x;

    constexpr Z(int v) : x(v) {}

    constexpr Z(Z&&) = default;
    constexpr auto operator=(Z&&) -> Z& = default;

    constexpr Z(Z const&) = delete;
    constexpr auto operator=(Z const&) -> Z& = delete;
};

constexpr static void move_only() {
    static_assert(di::concepts::Swappable<Z>);

    auto a = Z { 2 };
    auto b = Z { 4 };
    di::swap(a, b);

    ASSERT_EQ(a.x, 4);
    ASSERT_EQ(b.x, 2);
}

TESTC(util_swap, basic)
TESTC(util_swap, custom)
TESTC(util_swap, non_assignable)
TESTC(util_swap, move_only)
}
