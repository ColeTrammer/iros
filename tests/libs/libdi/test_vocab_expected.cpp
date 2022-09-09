#include <di/prelude.h>
#include <test/test.h>

constexpr void void_value() {
    auto x = di::Expected<void, int> {};
    EXPECT(x.has_value());
    EXPECT(!!x);

    EXPECT(x != di::Unexpected { 2 });
    EXPECT(x == di::Expected<void, short> {});
    EXPECT(x != di::Expected<void, short> { di::unexpect, 2 });

    x.value();
    x.emplace();

    x = di::Unexpected { 6 };
    EXPECT(x == di::Unexpected { 6 });

    static_assert(!di::concepts::ConvertibleTo<di::Expected<void, int>, void>);
    static_assert(di::concepts::detail::ExpectedCanConvertConstructor<void, long, void, int>);

    auto y = di::Expected<void, long>(x);
    EXPECT(y == di::Unexpected { 6 });

    auto z = di::Expected<void, long>(di::move(y)).transform([] -> void {});
    EXPECT(z == di::Unexpected { 6 });

    z.emplace();
    auto w = z.and_then([] {
        return di::Expected<void, char>(di::unexpect, 'a');
    });
    EXPECT(w == di::Unexpected { 'a' });

    z = di::Unexpected { 6 };
    auto v = z.or_else([](auto) {
        return di::Expected<void, long> {};
    });
    EXPECT(v.has_value());

    auto q = z.transform_or([](auto) {
        return 8;
    });
    EXPECT(q == di::Unexpected { 8 });
}

constexpr void void_error() {
    auto x = di::Expected { 2 };
    EXPECT_EQ(*x, 2);

    x = 8;
    EXPECT_EQ(*x, 8);

    EXPECT(x == 8);
    EXPECT(x != di::Unexpected { 8 });

    static_assert(di::concepts::Monad<di::Expected>);
    static_assert(di::concepts::MonadInstance<di::Expected<int, void>>);
    static_assert(di::concepts::MonadInstance<di::Expected<int, void>&>);

    auto y = x % [](auto y) {
        return y + 4;
    };
    EXPECT(y == 12);

    auto z = x % [](auto z) {
        return static_cast<long>(z + 2);
    };
    EXPECT(z == 10);

    auto w = x >> [](auto x) {
        return di::Expected<void, int> { di::unexpect, x };
    };
    EXPECT(w == di::Unexpected { 8 });
}

TEST_CONSTEXPR(vocab_expected, void_value, void_value)
TEST_CONSTEXPR(vocab_expected, void_error, void_error)
