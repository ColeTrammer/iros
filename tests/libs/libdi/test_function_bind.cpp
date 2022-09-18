#include <di/prelude.h>
#include <test/test.h>

struct M {
    int x;

    constexpr M(int xx) : x(xx) {}

    M(M const&) = delete;
    M(M&&) = default;
};

constexpr void front() {
    auto f = [](int x, int y, int z) {
        return x + y + z;
    };

    auto g = di::bind_front(f, 2, 4);
    ASSERT_EQ(g(2), 8);

    ASSERT(!di::concepts::Invocable<decltype(g), int, int>);

    auto h = [](auto const& x, auto const& y, auto const& z) {
        static_assert(di::SameAs<decltype(x), di::ReferenceWrapper<int> const&>);
        return x + y + z;
    };

    auto a = 2, b = 3;
    auto j = di::bind_front(h, di::ref(a), di::ref(b));
    ASSERT_EQ(j(4), 9);
}

constexpr void back() {
    auto f = [](int x, int y) {
        return x / y;
    };

    auto g = di::bind_back(f, 2);
    ASSERT_EQ(g(4), 2);

    auto h = [](M x, M y) {
        return x.x / y.x;
    };

    auto j = di::bind_back(h, M { 2 });
    ASSERT_EQ(di::move(j)(M { 4 }), 2);

    static_assert(di::concepts::Invocable<decltype(j)&&, M&&>);

    auto a = g;
    ASSERT_EQ(a(6), 3);
}

constexpr void compose() {
    auto f = [](int x, int y) {
        return x + y;
    };

    auto g = [](int x, int y, int z) {
        return x + y + z;
    };

    auto h = di::compose(di::bind_front(f, 2), di::bind_back(g, 4));
    ASSERT_EQ(h(3, 7), 16);

    ASSERT_EQ(2, 5 | di::piped([](int) {
                     return 2;
                 }));

    auto k = di::bind_front(f, 2) | di::piped(g);
    ASSERT_EQ(k(1, 2, 3), 8);
}

constexpr void pipeline() {
    auto f = [](int x, int y, int z) {
        return x + y + z;
    };
    auto g = di::bind_back(f, 1, 2);
    auto x = 1 | g;
    ASSERT_EQ(x, 4);
}

constexpr void curry() {
    auto f = [](int x, int y, int z) {
        return x + y + z;
    };
    ASSERT_EQ(6, 3 | di::curry(f)(1)(2));
}

constexpr void curry_back() {
    auto f = [](int x, int y, int z) {
        return x + y + z;
    };
    ASSERT_EQ(6, di::curry_back(f)(1)(2)(3));
}

TEST_CONSTEXPR(util_bind, front, front)
TEST_CONSTEXPR(util_bind, back, back)
TEST_CONSTEXPR(util_bind, compose, compose)
TEST_CONSTEXPR(util_bind, pipeline, pipeline)
TEST_CONSTEXPR(util_bind, curry, curry)
TEST_CONSTEXPR(util_bind, curry_back, curry_back)
