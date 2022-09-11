#include <di/prelude.h>
#include <di/util/rebindable_box.h>
#include <test/test.h>

constexpr void basic() {
    static_assert(di::concepts::Swappable<int>);
    static_assert(!di::concepts::SwappableWith<int, long>);
    static_assert(!di::concepts::SwappableWith<char, char const*>);

    int a = 13;
    int b = 26;
    di::swap(a, b);
    EXPECT_EQ(a, 26);
    EXPECT_EQ(b, 13);
}

struct X {
    int* p;

private:
    constexpr friend void tag_invoke(di::Tag<di::swap>, X& a, X& b) {
        di::swap(a.p, b.p);
        ++(*a.p);
    }
};

constexpr void custom() {
    static_assert(di::concepts::Swappable<X>);

    int c = 0;

    auto a = X { &c };
    auto b = X { &c };
    di::swap(a, b);

    EXPECT_EQ(c, 1);
}

struct Y {
    int x;

    constexpr Y(int v) : x(v) {}
    constexpr Y(Y&&) = default;
    constexpr Y& operator=(Y&&) = delete;
};

constexpr void non_assignable() {
    static_assert(!di::concepts::Swappable<Y>);
    static_assert(di::concepts::Swappable<di::util::RebindableBox<Y>>);

    auto a = di::util::RebindableBox { Y { 3 } };
    auto b = di::util::RebindableBox { Y { 7 } };
    di::swap(a, b);

    EXPECT_EQ(a.value().x, 7);
    EXPECT_EQ(b.value().x, 3);
}

struct Z {
    int x;

    constexpr Z(int v) : x(v) {}

    constexpr Z(Z&&) = default;
    constexpr Z& operator=(Z&&) = default;

    constexpr Z(Z const&) = delete;
    constexpr Z& operator=(Z const&) = delete;
};

constexpr void move_only() {
    static_assert(di::concepts::Swappable<Z>);

    auto a = Z { 2 };
    auto b = Z { 4 };
    di::swap(a, b);

    EXPECT_EQ(a.x, 4);
    EXPECT_EQ(b.x, 2);
}

TEST_CONSTEXPR(util_swap, basic, basic)
TEST_CONSTEXPR(util_swap, custom, custom)
TEST_CONSTEXPR(util_swap, non_assignable, non_assignable)
TEST_CONSTEXPR(util_swap, move_only, move_only)
