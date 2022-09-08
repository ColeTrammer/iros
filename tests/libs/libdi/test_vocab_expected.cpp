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

TEST_CONSTEXPR(vocab_expected, void_value, void_value)
