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
    EXPECT_EQ(g(2), 8);

    EXPECT(!di::concepts::Invocable<decltype(g), int, int>);

    auto h = [](auto const& x, auto const& y, auto const& z) {
        static_assert(di::SameAs<decltype(x), di::ReferenceWrapper<int> const&>);
        return x + y + z;
    };

    auto a = 2, b = 3;
    auto j = di::bind_front(h, di::ref(a), di::ref(b));
    EXPECT_EQ(j(4), 9);
}

constexpr void back() {
    auto f = [](int x, int y) {
        return x / y;
    };

    auto g = di::bind_back(f, 2);
    EXPECT_EQ(g(4), 2);

    auto h = [](M x, M y) {
        return x.x / y.x;
    };

    auto j = di::bind_back(h, M { 2 });
    EXPECT_EQ(di::move(j)(M { 4 }), 2);

    static_assert(di::concepts::Invocable<decltype(j)&&, M&&>);

    auto a = g;
    EXPECT_EQ(a(6), 3);
}

TEST_CONSTEXPR(util_bind, front, front)
TEST_CONSTEXPR(util_bind, back, back)
