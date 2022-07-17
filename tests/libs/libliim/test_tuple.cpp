#include <liim/pointers.h>
#include <liim/tuple.h>
#include <test/test.h>
#include <tuple>

constexpr void basic() {
    Tuple<int, long> x;
    EXPECT_EQ(x.get<0>(), 0);
    EXPECT_EQ(x.get<1>(), 0);

    x = { 3, 5l };
    EXPECT_EQ(x.get<0>(), 3);
    EXPECT_EQ(x.get<1>(), 5l);

    Tuple<int, UniquePtr<int>> y(5, make_unique<int>(10));
    EXPECT_EQ(y.get<0>(), 5);
    EXPECT_EQ(*y.get<1>(), 10);

    auto w = move(y).get<1>();
}

constexpr void destructing() {
    auto x = Tuple { 5, 7l };
    auto& [i, l] = x;
    EXPECT_EQ(i, 5);
    EXPECT_EQ(l, 7l);
    i = 4;
    EXPECT_EQ(x.get<0>(), 4);

    auto [a, b] = Tuple<int, UniquePtr<int>> { 8, make_unique<int>(15) };
    EXPECT_EQ(a, 8);
    EXPECT_EQ(*b, 15);

    auto z = Tuple<int&, long&> { x.get<0>(), x.get<1>() };
    auto c = z;
    EXPECT_EQ(c.get<0>(), 4);
    EXPECT_EQ(c.get<1>(), 7l);
}

constexpr void construct_piecewise() {
    auto x = Tuple<int, int, int>(piecewise_construct, Tuple(32), Tuple(42), Tuple(52));
    auto y = Tuple<int, int, int>::create(piecewise_construct, Tuple(Result<int, StringView>(32)), Tuple(Result<int, StringView>(42)),
                                          Tuple(Result<int, StringView>(52)));
    EXPECT(x == y);

    LIIM::MaybeUninit<Tuple<const int, int>> z;
    int a = 2;
    auto b = Result<int, StringView>(4);
    create_at(&z.value, piecewise_construct, Tuple<int&&>(move(a)), Tuple<Result<int, StringView>&&>(move(b)));

    auto w = Tuple<const int, int>::create(piecewise_construct, Tuple<int&&>(move(a)), Tuple<Result<int, StringView>&&>(move(b)));
}

constexpr void comparison() {
    auto x = Tuple { "abc"sv, "qwe" };
    auto y = Tuple { "abc", "qwe"sv };
    EXPECT(x == y);

    auto a = Tuple { 2, 4, 0.1 };
    auto b = Tuple { 1, 4, 0.1 };
    auto r = a <=> b;
    EXPECT(r == std::partial_ordering::greater);

    auto c = Tuple { "xxx"sv, 42 };
    auto d = Tuple { "xxx", 42l };
    auto w = c <=> d;
    EXPECT(w == std::strong_ordering::equal);
}

TEST_CONSTEXPR(tuple, basic, basic)
TEST_CONSTEXPR(tuple, destructing, destructing)
TEST_CONSTEXPR(tuple, construct_piecewise, construct_piecewise)
TEST_CONSTEXPR(tuple, comparison, comparison)
