#include <dius/test/prelude.h>

static i32 do_calc(i32 x) noexcept {
    return x + 2;
}

static i32 do_calc2(i32 x) {
    return x + 3;
}

static void function_ref_basic() {
    auto const f = di::FunctionRef { do_calc };
    ASSERT_EQ(f(5), 7);

    auto callable = [&](i32 x) -> i32 {
        return f(x) + 2;
    };
    auto const g = di::FunctionRef<i32(i32) const> { callable };
    ASSERT_EQ(g(5), 9);

    auto const h = di::FunctionRef { di::nontype<do_calc2> };
    ASSERT_EQ(h(5), 8);

    struct X {
        i32 h(i32 x) const { return x + y; }

        i32 y {};
    };

    auto const x = X { 5 };
    auto const k = di::FunctionRef { di::nontype<&X::h>, x };
    ASSERT_EQ(k(9), 14);

    auto const j = di::FunctionRef { di::nontype<&X::h>, di::address_of(x) };
    ASSERT_EQ(j(11), 16);

    static_assert(di::SameAs<decltype(f), di::FunctionRef<i32(i32) noexcept> const>);
    static_assert(di::SameAs<decltype(g), di::FunctionRef<i32(i32) const> const>);
    static_assert(di::SameAs<decltype(h), di::FunctionRef<i32(i32)> const>);
    static_assert(di::SameAs<decltype(k), di::FunctionRef<i32(i32)> const>);
    static_assert(di::SameAs<decltype(j), di::FunctionRef<i32(i32)> const>);
}

TEST(function_container, function_ref_basic)