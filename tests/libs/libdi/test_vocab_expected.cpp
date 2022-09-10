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

constexpr void basic() {
    auto x = di::Expected<int, int> { di::unexpect, 5 };
    EXPECT(x == di::Unexpected { 5 });

    x.emplace(7);
    EXPECT(x == 7);

    auto y = di::Expected<long, long>(x);
    EXPECT(y == 7);

    auto z = y % [](auto x) {
        return x + 2;
    } >> [](auto x) {
        return di::Expected<long, int> { di::unexpect, x - 2 };
    } << [](auto e) {
        return di::Expected<long long, void> { e };
    } & [](auto e) {
        return e - 1;
    };
    EXPECT(z == di::Expected { 7 });

    static_assert(di::concepts::TriviallyDestructible<di::Expected<int, int>>);
    static_assert(di::concepts::TriviallyCopyConstructible<di::Expected<int, int>>);
    static_assert(di::concepts::TriviallyMoveConstructible<di::Expected<int, int>>);
}

struct M {
    constexpr M(int x_) : x(x_) {}

    constexpr M(M const&) = delete;
    constexpr M(M&& xx) : x(di::exchange(xx.x, 0)) {}

    int x;

    constexpr friend bool operator==(M const& a, M const& b) { return a.x == b.x; }
};

constexpr void move_only() {
    auto x = di::Expected<M, int> { M(2) };
    EXPECT(x == M(2));

    auto y = di::Expected<M, long> { di::move(x) };
    EXPECT(y == M(2));
    EXPECT(x == M(0));

    auto z = di::move(y) % [](M x) {
        static_assert(di::SameAs<decltype(x), M>);
        return x;
    } >> [](M x) {
        static_assert(di::SameAs<decltype(x), M>);
        return di::Expected<M, int> { di::unexpect, x.x };
    };
    EXPECT(z == di::Unexpected { 2 });
}

constexpr void fallible() {
    auto x = 2;
    auto y = di::as_fallible(x);
    static_assert(di::SameAs<decltype(y), di::Expected<int, void>>);
    EXPECT_EQ(*y, 2);

    auto z = di::as_fallible(y);
    static_assert(di::SameAs<decltype(z), di::Expected<int, void>>);
    EXPECT_EQ(*z, 2);

    auto w = di::try_infallible(z);
    EXPECT_EQ(w, 2);

    auto a = di::Expected<int, int> { 2 };
    auto b = di::try_infallible(a);
    EXPECT(a == b);

    auto c = di::as_fallible(2) % [](auto x) {
        return x + 2;
    } >> [](auto z) {
        return di::Expected { z + 2 };
    } | di::try_infallible;
    EXPECT_EQ(c, 6);
}

constexpr void reference() {
    int a = 2;
    auto x = di::Expected { di::ref(a) };
    EXPECT_EQ(*x, a);

    int b = 3;
    auto y = di::Expected<int&, int&>(b);
    *y = 5;
    EXPECT_EQ(b, 5);
}

TEST_CONSTEXPR(vocab_expected, void_value, void_value)
TEST_CONSTEXPR(vocab_expected, void_error, void_error)
TEST_CONSTEXPR(vocab_expected, basic, basic)
TEST_CONSTEXPR(vocab_expected, move_only, move_only)
TEST_CONSTEXPR(vocab_expected, fallible, fallible)
TEST_CONSTEXPR(vocab_expected, reference, reference)
