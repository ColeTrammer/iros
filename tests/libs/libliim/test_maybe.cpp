#include <liim/maybe.h>
#include <test/test.h>

static_assert(Maybe<int> { 5 } == Maybe<int> { 5 });
static_assert(Maybe<int> { 5 } != Maybe<int> { 6 });
static_assert(Maybe<int> { 5 } != Maybe<int> {});
static_assert(Maybe<int> { 5 }.value() == 5);
static_assert(!Maybe<int> {}.has_value());

TEST(maybe, basic_getters) {
    auto none = Maybe<int> {};
    auto some = Maybe<int> { 5 };

    EXPECT(!none);
    EXPECT(!none.has_value());
    EXPECT(some);
    EXPECT(some.has_value());
    EXPECT_EQ(*some, 5);
    EXPECT_NOT_EQ(some, none);
    EXPECT_EQ(none.value_or(3), 3);
}

TEST(maybe, basic_setters) {
    auto none = Maybe<int> {};
    auto some = Maybe<int> { 5 };
    auto copy = some;
    auto moved = Maybe<int> { move(some) };

    EXPECT(!none);
    EXPECT(!some);
    EXPECT_EQ(copy.value_or(0), 5);
    EXPECT_EQ(moved.value_or(0), 5);
}

TEST(maybe, equivalence) {
    auto none = Maybe<int> {};
    auto one = Maybe<int> { 1 };
    auto two = Maybe<int> { 2 };
    EXPECT_EQ(none, Maybe<int> {});
    EXPECT_EQ(one, Maybe<int> { 1 });
    EXPECT_EQ(two, 2);
    EXPECT_NOT_EQ(one, two);
    EXPECT_NOT_EQ(one, none);
}

TEST(maybe, functional) {
    auto none = Maybe<int> {};
    auto one = Maybe<int> { 1 };

    EXPECT_EQ(Maybe<int> {}, none.map([](auto) {
        return 2;
    }));
    EXPECT_EQ(2, one.map([](auto) {
        return 2;
    }));

    EXPECT_EQ(Maybe<int> {}, none.and_then([](auto) -> Maybe<int> {
        return { 3 };
    }));
    EXPECT_EQ(Maybe<int> {}, one.and_then([](auto) -> Maybe<int> {
        return {};
    }));
    EXPECT_EQ(3, one.and_then([](auto) -> Maybe<int> {
        return { 3 };
    }));
}

TEST(maybe, references) {
    int v = 42;
    auto x = Maybe<int&> { v };
    EXPECT_EQ(x.value(), 42);

    int w = 42;
    auto y = Maybe<int&> { w };
    EXPECT_EQ(y.value(), 42);

    EXPECT_NOT_EQ(x, y);

    v = 11;
    EXPECT_EQ(x.value(), 11);
}
