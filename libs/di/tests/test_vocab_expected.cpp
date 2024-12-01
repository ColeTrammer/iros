#include <di/vocab/expected/prelude.h>
#include <dius/test/prelude.h>

namespace vocab_expected {
constexpr static void void_value() {
    auto x = di::Expected<void, int> {};
    ASSERT(x.has_value());
    ASSERT(!!x);

    ASSERT_NOT_EQ(x, di::Unexpected { 2 });
    ASSERT_EQ(x, (di::Expected<void, short> {}));
    ASSERT_NOT_EQ(x, (di::Expected<void, short> { di::unexpect, 2 }));

    x.value();
    x.emplace();

    x = di::Unexpected { 6 };
    ASSERT_EQ(x, di::Unexpected { 6 });

    static_assert(!di::concepts::ConvertibleTo<di::Expected<void, int>, void>);
    static_assert(di::concepts::detail::ExpectedCanConvertConstructor<void, long, void, int>);

    auto y = di::Expected<void, long>(x);
    ASSERT_EQ(y, di::Unexpected { 6 });

    auto z = di::Expected<void, long>(di::move(y)).transform([] -> void {});
    ASSERT_EQ(z, di::Unexpected { 6 });

    z.emplace();
    auto w = z.and_then([] {
        return di::Expected<void, char>(di::unexpect, 'a');
    });
    ASSERT_EQ(w, di::Unexpected { 'a' });

    z = di::Unexpected { 6 };
    auto v = z.or_else([](auto) {
        return di::Expected<void, long> {};
    });
    ASSERT(v.has_value());

    auto q = z.transform_error([](auto) {
        return 8;
    });
    ASSERT_EQ(q, di::Unexpected { 8 });
}

constexpr static void void_error() {
    auto x = di::Expected { 2 };
    ASSERT_EQ(*x, 2);

    x = 8;
    ASSERT_EQ(*x, 8);

    ASSERT_EQ(x, 8);
    ASSERT_NOT_EQ(x, di::Unexpected { 8 });

    static_assert(di::concepts::Monad<di::Expected>);
    static_assert(di::concepts::MonadInstance<di::Expected<int, void>>);
    static_assert(di::concepts::MonadInstance<di::Expected<int, void>&>);
    static_assert(di::concepts::MonadInstance<di::Expected<int, void>&&>);
    static_assert(di::concepts::MonadInstance<di::Expected<int&, void>&>);
    static_assert(di::concepts::MonadInstance<di::Expected<int&, void>&&>);

    auto y = x % [](auto y) {
        return y + 4;
    };
    ASSERT_EQ(y, 12);

    auto z = x % [](auto z) {
        return static_cast<long>(z) + 2;
    };
    ASSERT_EQ(z, 10);

    auto w = x >> [](auto x) {
        return di::Expected<void, int> { di::unexpect, x };
    };
    ASSERT_EQ(w, di::Unexpected { 8 });
}

constexpr static void basic() {
    auto x = di::Expected<int, int> { di::unexpect, 5 };
    ASSERT_EQ(x, di::Unexpected { 5 });

    x.emplace(7);
    ASSERT_EQ(x, 7);

    auto y = di::Expected<long, long>(x);
    ASSERT_EQ(y, 7);

    auto z = y % [](auto x) {
        return x + 2;
    } >> [](auto x) {
        return di::Expected<long, int> { di::unexpect, x - 2 };
    } << [](auto e) {
        return di::Expected<long long, void> { e };
    } & [](auto e) {
        return e - 1;
    };
    ASSERT_EQ(z, di::Expected { 7 });

    static_assert(di::concepts::TriviallyDestructible<di::Expected<int, int>>);
    static_assert(di::concepts::TriviallyCopyConstructible<di::Expected<int, int>>);
    static_assert(di::concepts::TriviallyMoveConstructible<di::Expected<int, int>>);
}

struct M {
    constexpr M(int x_) : x(x_) {}

    constexpr M(M const&) = delete;
    constexpr M(M&& xx) : x(di::exchange(xx.x, 0)) {}

    int x;

    constexpr friend auto operator==(M const& a, M const& b) -> bool { return a.x == b.x; }
};

constexpr static void move_only() {
    auto x = di::Expected<M, int> { M(2) };
    ASSERT_EQ(x, M(2));

    auto y = di::Expected<M, long> { di::move(x) };
    ASSERT_EQ(y, M(2));
    ASSERT_EQ(x, M(0));

    auto z = di::move(y) % [](M x) {
        static_assert(di::SameAs<decltype(x), M>);
        return x;
    } >> [](M x) {
        static_assert(di::SameAs<decltype(x), M>);
        return di::Expected<M, int> { di::unexpect, x.x };
    };
    ASSERT_EQ(z, di::Unexpected { 2 });
}

constexpr static void fallible() {
    auto x = 2;
    auto y = di::as_fallible(auto(x));
    static_assert(di::SameAs<decltype(y), di::Expected<int, void>>);
    ASSERT_EQ(*y, 2);

    auto z = di::as_fallible(y);
    static_assert(di::SameAs<decltype(z), di::Expected<int, void>>);
    ASSERT_EQ(*z, 2);

    auto w = di::try_infallible(z);
    ASSERT_EQ(w, 2);

    auto a = di::Expected<int, int> { 2 };
    auto b = di::try_infallible(a);
    ASSERT_EQ(a, b);

    auto c = di::as_fallible(2) % [](auto x) {
        return x + 2;
    } >> [](auto z) {
        return di::Expected { z + 2 };
    } | di::try_infallible;
    ASSERT_EQ(c, 6);
}

constexpr static void reference() {
    int a = 2;
    auto x = di::Expected { di::ref(a) };
    ASSERT_EQ(*x, a);

    int b = 3;
    auto y = di::Expected<int&, int&>(b);
    *y = 5;
    ASSERT_EQ(b, 5);
}

TESTC(vocab_expected, void_value)
TESTC(vocab_expected, void_error)
TESTC(vocab_expected, basic)
TESTC(vocab_expected, move_only)
TESTC(vocab_expected, fallible)
TESTC(vocab_expected, reference)
}
