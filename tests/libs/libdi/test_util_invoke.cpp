#include <di/util/concepts/same_as.h>
#include <di/util/invoke.h>
#include <di/util/prelude.h>
#include <test/test.h>

struct X {
    int y;
    constexpr int z(int p) const { return y + p; }
};

struct Y : X {};

struct Z {};

constexpr void function() {
    auto f = [](int y, int z) -> int {
        return y + z;
    };

    static_assert(di::conc::Invocable<decltype(f), int, int>);
    static_assert(di::conc::Invocable<decltype(f), int, short>);
    static_assert(di::conc::InvocableTo<decltype(f), int, int, short>);
    static_assert(di::conc::InvocableTo<decltype(f), long, int, short>);
    static_assert(!di::conc::InvocableTo<decltype(f), char const*, int, short>);
    static_assert(!di::conc::InvocableTo<decltype(f), Z, int, short>);
    static_assert(!di::conc::Invocable<decltype(f), int, char const*>);
    static_assert(!di::conc::Invocable<decltype(f), int, int, int>);
    static_assert(di::conc::SameAs<int, di::meta::InvokeResult<decltype(f), int, int>>);

    EXPECT_EQ(di::invoke(f, 3, 6), 9);
}

constexpr void member_object() {
    auto f = &X::y;

    static_assert(di::conc::Invocable<decltype(f), X>);
    static_assert(di::conc::Invocable<decltype(f), Y>);
    static_assert(!di::conc::Invocable<decltype(f), Z>);
    static_assert(!di::conc::Invocable<decltype(f)>);
    static_assert(!di::conc::Invocable<decltype(f), X, X>);
    static_assert(di::conc::SameAs<int const&, di::meta::InvokeResult<decltype(f), X const&>>);

    auto x = X { 42 };
    EXPECT_EQ(di::invoke(f, x), 42);
    EXPECT_EQ(di::invoke(f, &x), 42);

    auto y = Y { 13 };
    EXPECT_EQ(di::invoke(f, y), 13);
    EXPECT_EQ(di::invoke(f, &y), 13);
}

constexpr void member_function() {
    auto f = &X::z;

    static_assert(di::conc::Invocable<decltype(f), X, int>);
    static_assert(di::conc::Invocable<decltype(f), Y, int>);
    static_assert(!di::conc::Invocable<decltype(f), Z, int>);
    static_assert(!di::conc::Invocable<decltype(f), X>);
    static_assert(!di::conc::Invocable<decltype(f), X, int, int>);
    static_assert(di::conc::SameAs<int, di::meta::InvokeResult<decltype(f), X, int>>);

    auto x = X { 42 };
    EXPECT_EQ(di::invoke(f, x, 5), 47);
    EXPECT_EQ(di::invoke(f, &x, 5), 47);

    auto y = Y { 13 };
    EXPECT_EQ(di::invoke(f, y, 3), 16);
    EXPECT_EQ(di::invoke(f, &y, 3), 16);
}

TEST_CONSTEXPR(util_invoke, function, function)
TEST_CONSTEXPR(util_invoke, member_object, member_object)
TEST_CONSTEXPR(util_invoke, member_function, member_function)
