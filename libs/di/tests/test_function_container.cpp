#include <di/function/container/prelude.h>
#include <di/util/is_constant_evaluated.h>
#include <dius/test/prelude.h>

namespace function_container {
constexpr static auto do_calc(i32 x) noexcept -> i32 {
    return x + 2;
}

constexpr static auto do_calc2(i32 x) -> i32 {
    return x + 3;
}

constexpr static void function_ref_basic() {
    if (!di::is_constant_evaluated()) {
        auto const f = di::FunctionRef { do_calc };
        ASSERT_EQ(f(5), 7);
    }

    auto callable = [&](i32 x) -> i32 {
        return do_calc(x) + 2;
    };
    auto const g = di::FunctionRef<i32(i32) const> { callable };
    ASSERT_EQ(g(5), 9);

    auto const h = di::FunctionRef { di::c_<do_calc2> };
    ASSERT_EQ(h(5), 8);

    struct X {
        constexpr auto h(i32 x) const -> i32 { return x + y; }

        i32 y {};
    };

    auto const x = X { 5 };
    auto const k = di::FunctionRef { di::c_<&X::h>, x };
    ASSERT_EQ(k(9), 14);

    auto const j = di::FunctionRef { di::c_<&X::h>, di::addressof(x) };
    ASSERT_EQ(j(11), 16);

    // static_assert(di::SameAs<decltype(f), di::FunctionRef<i32(i32) noexcept> const>);
    // static_assert(di::SameAs<decltype(g), di::FunctionRef<i32(i32) const> const>);
    static_assert(di::SameAs<decltype(h), di::FunctionRef<i32(i32)> const>);
    static_assert(di::SameAs<decltype(k), di::FunctionRef<i32(i32)> const>);
    static_assert(di::SameAs<decltype(j), di::FunctionRef<i32(i32)> const>);
}

constexpr static void function_basic() {
    auto f = di::Function<i32(i32)> { [](i32 x) -> i32 {
        return x + 3;
    } };
    ASSERT(f);
    ASSERT_NOT_EQ(f, nullptr);
    ASSERT_EQ(f(5), 8);

    f = do_calc;
    ASSERT_EQ(f(6), 8);

    f = nullptr;
    ASSERT(!f);

    auto const g = di::Function<i32(i32) const> { di::c_<do_calc2> };
    ASSERT(g);
    ASSERT_EQ(g(1), 4);

    struct X {
        constexpr auto h(i32 x) const -> i32 { return x + y; }

        i32 y {};
    };

    auto const x = X { 5 };
    auto const k = di::Function<i32(i32) const> { di::c_<&X::h>, x };
    ASSERT_EQ(k(9), 14);

    auto const j = di::Function<i32(i32) const> { di::c_<&X::h>, di::addressof(x) };
    ASSERT_EQ(j(11), 16);

    auto const lambda = [x = 2, y = 4, z = 6, n = 1, p = 3](i32 m) {
        return x + y + z + n + m + p;
    };
    static_assert(!di::concepts::ConstructibleFrom<di::Function<i32(i32) const>, decltype(lambda)>);
    auto const n = di::make_function<i32(i32) const>(lambda);
    ASSERT_EQ(n(1), 17);
}

TESTC_GCC_NOSAN(function_container, function_ref_basic)
TESTC_GCC_NOSAN(function_container, function_basic)
}
