#include <di/function/prelude.h>
#include <dius/test/prelude.h>

namespace function_invoke {
struct X {
    int y;
    constexpr auto z(int p) const -> int { return y + p; }
};

struct Y : X {};

struct Z {};

constexpr static void function() {
    auto f = [](int y, int z) -> int {
        return y + z;
    };

    static_assert(di::concepts::Invocable<decltype(f), int, int>);
    static_assert(di::concepts::Invocable<decltype(f), int, short>);
    static_assert(di::concepts::InvocableTo<decltype(f), int, int, short>);
    static_assert(di::concepts::InvocableTo<decltype(f), long, int, short>);
    static_assert(!di::concepts::InvocableTo<decltype(f), char const*, int, short>);
    static_assert(!di::concepts::InvocableTo<decltype(f), Z, int, short>);
    static_assert(!di::concepts::Invocable<decltype(f), int, char const*>);
    static_assert(!di::concepts::Invocable<decltype(f), int, int, int>);
    static_assert(di::concepts::SameAs<int, di::meta::InvokeResult<decltype(f), int, int>>);

    ASSERT_EQ(di::invoke(f, 3, 6), 9);
}

constexpr static void member_object() {
    auto f = &X::y;

    static_assert(di::concepts::Invocable<decltype(f), X>);
    static_assert(di::concepts::Invocable<decltype(f), Y>);
    static_assert(di::concepts::Invocable<decltype(f), di::ReferenceWrapper<Y>>);
    static_assert(!di::concepts::Invocable<decltype(f), Z>);
    static_assert(!di::concepts::Invocable<decltype(f)>);
    static_assert(!di::concepts::Invocable<decltype(f), X, X>);
    static_assert(di::concepts::SameAs<int const&, di::meta::InvokeResult<decltype(f), X const&>>);

    auto x = X { 42 };
    ASSERT_EQ(di::invoke(f, x), 42);
    ASSERT_EQ(di::invoke(f, di::cref(di::ref(x))), 42);
    ASSERT_EQ(di::invoke(f, &x), 42);

    auto y = Y { 13 };
    ASSERT_EQ(di::invoke(f, y), 13);
    ASSERT_EQ(di::invoke(f, di::cref(y)), 13);
    ASSERT_EQ(di::invoke(f, &y), 13);
}

constexpr static void member_function() {
    auto f = &X::z;

    static_assert(di::concepts::Invocable<decltype(f), X, int>);
    static_assert(di::concepts::Invocable<decltype(f), di::ReferenceWrapper<Y>, int>);
    static_assert(di::concepts::Invocable<decltype(f), Y, int>);
    static_assert(!di::concepts::Invocable<decltype(f), Z, int>);
    static_assert(!di::concepts::Invocable<decltype(f), X>);
    static_assert(!di::concepts::Invocable<decltype(f), X, int, int>);
    static_assert(di::concepts::SameAs<int, di::meta::InvokeResult<decltype(f), X, int>>);

    auto x = X { 42 };
    ASSERT_EQ(di::invoke(&X::z, x, 5), 47);
    ASSERT_EQ(di::invoke(&X::z, di::ref(x), 5), 47);
    ASSERT_EQ(di::invoke(&X::z, &x, 5), 47);

    auto y = Y { 13 };
    ASSERT_EQ(di::invoke(&X::z, y, 3), 16);
    ASSERT_EQ(di::invoke(&X::z, di::cref(y), 3), 16);
    ASSERT_EQ(di::invoke(&X::z, &y, 3), 16);
}

constexpr static void invoke_r_void() {
    auto f = [] {
        return 32;
    };

    static_assert(di::concepts::InvocableTo<decltype(f), void>);

    di::invoke_r<void>(f);
}

TESTC(util_invoke, function)
TESTC(util_invoke, member_object)
TESTC(util_invoke, member_function)
TESTC(util_invoke, invoke_r_void)
}
