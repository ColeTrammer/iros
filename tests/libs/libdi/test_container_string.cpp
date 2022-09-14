#include <di/concepts/always_true.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = "QWER"_sv;
    (void) x;

    EXPECT(x.size() == 4);

    int c = 0;
    for (auto xx : x) {
        (void) xx;
        c++;
    }
    EXPECT(c == 4);
}

constexpr void push_back() {
    auto x = di::String {};
    x.push_back('a');
    x.push_back('b');
    x.push_back('c');
    EXPECT(x.size() == 3);
}

#define DI_COMPILE_STRING(exp)                                                            \
    [&] {                                                                                 \
        using __CodeUnit = di::meta::EncodingCodeUnit<di::meta::Encoding<decltype(exp)>>; \
        constexpr auto __size = (exp).size();                                             \
        auto __string = (exp);                                                            \
        auto __span = di::Span<__CodeUnit const, __size>(__string.span());                \
        return di::container::FixedString<__CodeUnit, __size> { __span };                 \
    }()

template<typename F, typename G>
consteval void check_equal() {
    auto f = F();
    auto g = G();
    if consteval {
        if constexpr (f() != g()) {
            constexpr auto a_str = DI_COMPILE_STRING(di::to_string(f()));
            constexpr auto b_str = DI_COMPILE_STRING(di::to_string(g()));
            static_assert(di::concepts::AlwaysTrue<di::meta::AssertFail<a_str, b_str>>);
        }
    } else {
        assert(f() == g());
    }
}

constexpr void fixed() {
    constexpr auto function = [] {
        auto a = di::String {};
        a.push_back('a');
        a.push_back('a');
        a.push_back('b');
        return a;
    };

    // DI_ASSERT(false);

    check_equal<decltype(function), decltype(function)>();

    // constexpr auto string = DI_COMPILE_STRING(function());
    // constexpr auto other =
    // DI_COMPILE_STRING((di::StringView { "The required expression '4' != '3'", sizeof("The required expression '4' != '3'") - 1 }));

    // static_assert(di::meta::AlwaysTrue<di::meta::AssertFail<other>>);

    // static_assert(di::meta::AlwaysTrue<di::meta::AssertEqual<string, other>>);
}

TEST_CONSTEXPR(container_string, basic, basic)
TEST_CONSTEXPR(container_string, push_back, push_back)
TEST_CONSTEXPR(container_string, fixed, fixed)
