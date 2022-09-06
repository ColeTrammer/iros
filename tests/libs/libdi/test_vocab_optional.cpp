#include <di/concepts/trivial.h>
#include <di/concepts/trivially_default_constructible.h>
#include <di/prelude.h>
#include <test/test.h>

static_assert(sizeof(di::Optional<int&>) == sizeof(int*));

constexpr void basic() {
    auto x = di::Optional<int> {};
    EXPECT(!x.has_value());

    EXPECT_EQ(x.value_or(4), 4);

    EXPECT_EQ(x.emplace(8), 8);
    EXPECT_EQ(x.value(), 8);

    x = 3;
    EXPECT_EQ(x.value(), 3);

    x.reset();
    EXPECT(!x);

    auto y = di::Optional<int>(2);
    EXPECT_EQ(*y, 2);
}

constexpr void conversions() {
    struct X {
        int x;
    };

    struct Y {
        int y;

        constexpr explicit operator X() const { return X { y }; }
    };

    auto x = di::Optional<X>(di::in_place, 3);
    EXPECT_EQ(x.value().x, 3);

    auto y = di::Optional<Y>(di::in_place, 4);
    x = y;
    EXPECT_EQ(x.value().x, 4);

    auto z = di::Optional<X>(y);
    EXPECT_EQ(z.value().x, 4);

    z = di::Optional<Y>(di::in_place, 5);
    EXPECT_EQ(z.value().x, 5);
}

constexpr void make_optional() {
    auto x = di::make_optional(5);
    EXPECT_EQ(x.value(), 5);

    struct Y {
        int x, y;
    };
    auto y = di::make_optional<Y>(4, 3);
    EXPECT_EQ(y->x + y->y, 7);

    int b = 2;
    auto a = di::make_optional(di::ref(b));
    *a = 3;
    EXPECT_EQ(b, 3);
}

constexpr void references() {
    int i = 32;
    auto x = di::make_optional(di::ref(i));
    EXPECT_EQ(*x, 32);

    int j = 42;
    x = j;
    EXPECT_EQ(*x, 42);
    EXPECT_EQ(i, 32);
}

struct Z {
    constexpr Z() = delete;
    constexpr Z(const Z&) = delete;
    constexpr Z(Z&&) = delete;

    constexpr Z& operator&(const Z&) = delete;
    constexpr Z& operator&(Z&&) = delete;
};

constexpr void trivial() {
    static_assert(!di::concepts::TriviallyDefaultConstructible<di::Optional<int>>);
    static_assert(di::concepts::TriviallyCopyConstructible<di::Optional<int>>);
    static_assert(di::concepts::TriviallyMoveConstructible<di::Optional<int>>);
    static_assert(di::concepts::TriviallyCopyAssignable<di::Optional<int>>);
    static_assert(di::concepts::TriviallyMoveAssignable<di::Optional<int>>);
    static_assert(di::concepts::TriviallyDestructible<di::Optional<int>>);
    static_assert(!di::concepts::Trivial<di::Optional<int>>);

    static_assert(!di::concepts::TriviallyDefaultConstructible<di::Optional<Z&>>);
    static_assert(di::concepts::TriviallyCopyConstructible<di::Optional<Z&>>);
    static_assert(di::concepts::TriviallyMoveConstructible<di::Optional<Z&>>);
    static_assert(di::concepts::TriviallyCopyAssignable<di::Optional<Z&>>);
    static_assert(di::concepts::TriviallyMoveAssignable<di::Optional<Z&>>);
    static_assert(di::concepts::TriviallyDestructible<di::Optional<Z&>>);
    static_assert(!di::concepts::Trivial<di::Optional<Z&>>);

    auto y = di::Optional<Z&>();
    auto x = y;
    auto z = y;
    EXPECT(!x.has_value());
    EXPECT(!y.has_value());
    EXPECT(!z.has_value());
}

struct M {
    constexpr M() = default;
    constexpr ~M() = default;

    constexpr M(M const&) = delete;
    constexpr M(M&&) = default;

    constexpr M& operator=(M const&) = delete;
    constexpr M& operator=(M&&) = default;
};

constexpr void monad() {
    auto x = di::Optional<int>();
    auto y = x.or_else([] {
        return di::make_optional<int>(42);
    });
    EXPECT_EQ(y.value(), 42);

    auto z = y.transform([](auto x) {
        return x + 1;
    });
    EXPECT_EQ(z.value(), 43);

    auto w = z.and_then([](auto) -> di::Optional<Z&> {
        return di::nullopt;
    });
    EXPECT(!w);

    auto m = di::Optional<M>();
    auto qq = di::move(m)
                  .or_else([] {
                      return di::make_optional<M>();
                  })
                  .and_then([](M) {
                      return di::make_optional<M>();
                  })
                  .transform([](auto&&) -> int {
                      return 2;
                  });

    int zz = 4;
    int zzz = 8;
    auto a = di::make_optional(di::ref(zz));
    a = a.and_then([&](auto&) {
        return di::make_optional(di::ref(zzz));
    });
    EXPECT_EQ(*a, zzz);

    static_assert(di::concepts::Monad<di::Optional>);
    static_assert(di::concepts::MonadInstance<di::Optional<int>>);

    auto yyy = di::unit<di::Optional>(5) % [](auto i) {
        return i + 5;
    } % [](auto x) {
        return x + 1;
    } % [](auto j) {
        return j - 2;
    } >> [](auto k) {
        return di::unit<di::Optional>(k + 2);
    };
    EXPECT_EQ(*yyy, 11);
}

constexpr void swap() {
    auto x = di::make_optional(3);
    auto y = di::make_optional(7);
    di::swap(x, y);

    EXPECT_EQ(*x, 7);
    EXPECT_EQ(*y, 3);
}

struct X {};

constexpr void compare() {
    static_assert(di::concepts::EqualityComparable<di::Optional<int>>);
    static_assert(di::concepts::EqualityComparableWith<di::Optional<int>, di::Optional<long>>);
    static_assert(di::concepts::EqualityComparableWith<di::Optional<int>, di::vocab::NullOpt>);
    static_assert(di::concepts::EqualityComparableWith<di::Optional<int>, int>);
    static_assert(di::concepts::EqualityComparableWith<di::Optional<di::Optional<int>>, di::Optional<int>>);
    static_assert(di::concepts::EqualityComparableWith<di::Optional<di::Optional<int>>, int>);
    static_assert(!di::concepts::EqualityComparableWith<X, X>);
    static_assert(!di::concepts::EqualityComparableWith<di::Optional<X>, di::Optional<X>>);
    auto x = di::make_optional(3);
    auto y = di::make_optional(3l);
    auto z = di::Optional<di::Optional<di::Optional<int>>> { di::in_place, di::in_place, di::in_place, 2 };

    EXPECT(x == y);
    EXPECT(x != di::nullopt);
    EXPECT(x == 3);

    EXPECT(z == 2);
    EXPECT(z != 1);

    static_assert(di::concepts::ThreeWayComparable<di::Optional<int>>);
    static_assert(di::concepts::ThreeWayComparableWith<di::Optional<int>, di::Optional<long>>);
    static_assert(di::concepts::ThreeWayComparableWith<di::Optional<int>, di::vocab::NullOpt>);
    static_assert(di::concepts::ThreeWayComparableWith<di::Optional<int>, int>);
    static_assert(di::concepts::ThreeWayComparableWith<di::Optional<di::Optional<int>>, di::Optional<int>>);
    static_assert(di::concepts::ThreeWayComparableWith<di::Optional<di::Optional<int>>, int>);
    static_assert(!di::concepts::ThreeWayComparableWith<X, X>);
    static_assert(!di::concepts::ThreeWayComparableWith<di::Optional<X>, di::Optional<X>>);

    EXPECT(x <= y);
    EXPECT(x < di::make_optional(4l));
    EXPECT(x > di::Optional<int>(di::nullopt));
    EXPECT(x > di::nullopt);
    EXPECT(x >= 2);

    EXPECT(z < 3);
    EXPECT(z > 1);
}

constexpr void container() {
    auto x = di::make_optional(2);
    EXPECT(!x.empty());
    EXPECT_EQ(x.size(), 1u);
    for (auto y : x) {
        EXPECT_EQ(y, 2);
    }

    auto& y = *x.front();
    EXPECT_EQ(y, 2);

    auto z = di::make_optional(di::ref(*x));
    for (auto& y : x) {
        EXPECT_EQ(y, 2);
        y = 3;
    }

    EXPECT_EQ(*x, 3);

    auto a = di::Optional<int> {};
    for (auto y : a) {
        (void) y;
        EXPECT(false);
    }
}

TEST_CONSTEXPR(vocab_optional, basic, basic)
TEST_CONSTEXPR(vocab_optional, conversions, conversions)
TEST_CONSTEXPR(vocab_optional, make_optional, make_optional)
TEST_CONSTEXPR(vocab_optional, references, references)
TEST_CONSTEXPR(vocab_optional, trivial, trivial)
TEST_CONSTEXPR(vocab_optional, monad, monad)
TEST_CONSTEXPR(vocab_optional, swap, swap)
TEST_CONSTEXPR(vocab_optional, compare, compare)
TEST_CONSTEXPR(vocab_optional, container, container)
