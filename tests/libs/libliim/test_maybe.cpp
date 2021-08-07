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
    EXPECT(*some == 5);
    EXPECT(some != none);
    EXPECT(none.value_or(3) == 3);
}

TEST(maybe, basic_setters) {
    auto none = Maybe<int> {};
    auto some = Maybe<int> { 5 };
    auto copy = some;
    auto moved = Maybe<int> { move(some) };

    EXPECT(!none);
    EXPECT(!some);
    EXPECT(copy.value_or(0) == 5);
    EXPECT(moved.value_or(0) == 5);
}

TEST(maybe, equivalence) {
    auto none = Maybe<int> {};
    auto one = Maybe<int> { 1 };
    auto two = Maybe<int> { 2 };
    EXPECT(none == Maybe<int> {});
    EXPECT(one == Maybe<int> { 1 });
    EXPECT(two == Maybe<int> { 2 });
    EXPECT(one != two);
    EXPECT(one != none);
}

TEST(maybe, functional) {
    auto none = Maybe<int> {};
    auto one = Maybe<int> { 1 };

    EXPECT(none.map([](auto) {
        return 2;
    }) == Maybe<int> {});
    EXPECT(one.map([](auto) {
        return 2;
    }) == Maybe<int> { 2 });

    EXPECT(none.and_then([](auto) -> Maybe<int> {
        return { 3 };
    }) == Maybe<int> {});
    EXPECT(one.and_then([](auto) -> Maybe<int> {
        return {};
    }) == Maybe<int> {});
    EXPECT(one.and_then([](auto) -> Maybe<int> {
        return { 3 };
    }) == Maybe<int> { 3 });
}
