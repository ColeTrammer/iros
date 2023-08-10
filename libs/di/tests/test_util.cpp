#include <di/util/named_arguments.h>
#include <di/util/prelude.h>
#include <dius/test/prelude.h>

namespace util {
constexpr void scope_exit() {
    int value = 5;
    {
        auto guard = di::ScopeExit([&] {
            value = 42;
        });

        auto o = di::move(guard);
    }
    ASSERT_EQ(value, 42);

    {
        auto guard = di::ScopeExit([&] {
            value = 5;
        });
        guard.release();
    }
    ASSERT_EQ(value, 42);
}

constexpr void uuid() {
    // Standard Endian Format.
    auto x = "00112233-4455-6677-8899-aabbccddeeff"_uuid;
    auto ex = di::Array {
        0x00_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b, 0x55_b, 0x66_b, 0x77_b,
        0x88_b, 0x99_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 0xee_b, 0xff_b,
    };

    // Little Endian Format.
    auto y = "00112233-4455-6677-c899-AABBCCDDEEFF"_uuid;
    auto ey = di::Array {
        0x33_b, 0x22_b, 0x11_b, 0x00_b, 0x55_b, 0x44_b, 0x77_b, 0x66_b,
        0xc8_b, 0x99_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 0xee_b, 0xff_b,
    };

    auto ds = di::to_string(x);
    auto es = di::to_string(y);

    ASSERT_EQ(x, di::UUID(ex));
    ASSERT_EQ(ds, "00112233-4455-6677-8899-aabbccddeeff"_sv);
    ASSERT_EQ(y, di::UUID(ey));
    ASSERT_EQ(es, "00112233-4455-6677-c899-aabbccddeeff"_sv);
    ASSERT_NOT_EQ(x, di::UUID());
    ASSERT_NOT_EQ(y, di::UUID());
    ASSERT(!x.null());
    ASSERT(!y.null());
}

constexpr void strong_int() {
    struct XTag {
        using Type = i32;
        struct Mixin {
            using Self = di::StrongInt<XTag>;

            constexpr i32 foo() const { return static_cast<Self const&>(*this).raw_value() + 1; }
        };
    };

    using X = di::StrongInt<XTag>;

    auto x = X(0);
    ASSERT_EQ(x, X(0));

    ++x;
    ASSERT_EQ(x, X(1));

    ASSERT_EQ(x.foo(), 2);

    auto xs = di::to_string(x);
    ASSERT_EQ(xs, "1"_sv);

    static_assert(sizeof(X) == sizeof(i32));
}

//! [named_arguments]
struct Foo : di::NamedArgument<Foo, i32> {};
struct Bar : di::NamedArgument<Bar, i32> {};
template<di::concepts::Integral T>
struct Baz : di::NamedArgument<di::InPlaceTemplate<Baz>, T> {};
template<typename T>
Baz(T&&) -> Baz<T>;

template<typename... Args>
requires(di::ValidNamedArguments<di::meta::List<Foo, Bar, di::InPlaceTemplate<Baz>>, Args...>)
constexpr i32 f(Args&&... args) {
    auto named = di::NamedArguments(di::forward<Args>(args)...);

    auto foo_value = di::get_named_argument_or<Foo>(di::move(named), 1);
    auto bar_value = di::get_named_argument_or<Bar>(di::move(named), 2);
    auto baz_value = di::get_named_argument_or<di::InPlaceTemplate<Baz>>(di::move(named), 3);

    return foo_value + bar_value + baz_value;
}

constexpr static void named_arguments() {
    ASSERT_EQ(f(), 6);
    ASSERT_EQ(f(Foo(5)), 10);
    ASSERT_EQ(f(Bar(5)), 9);
    ASSERT_EQ(f(Foo(5), Bar(5)), 13);
    ASSERT_EQ(f(Foo(5), Bar(5), Baz(5)), 15);
}
//! [named_arguments]

TESTC(util, scope_exit)
TESTC(util, uuid)
TESTC(util, strong_int)
}
