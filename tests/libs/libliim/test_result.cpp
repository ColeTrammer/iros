#include <liim/result.h>
#include <liim/string.h>
#include <test/test.h>

TEST(result, basic_getters) {
    auto ok = Result<int, String> { 5 };
    auto err = Result<int, String> { "hello"s };

    EXPECT(ok);
    EXPECT(!!ok);
    EXPECT(!err);

    EXPECT_EQ(ok.value(), 5);
    EXPECT_EQ(err.error(), "hello"s);

    EXPECT_EQ(ok, ok);
}

TEST(result, functional) {
    auto ok = Result<int, String> { 5 };
    auto err = Result<int, String> { "error"s };

    EXPECT_EQ((Result<int, String> { 10 }), ok.map([](auto x) {
        return x * 2;
    }));

    EXPECT_EQ((Result<int, String> { "error"s }), err.map([](auto x) {
        return x * 2;
    }));

    EXPECT_EQ((Result<int, String> { 5 }), ok.map_error([](auto x) {
        return format("{} plus", x);
    }));

    EXPECT_EQ((Result<int, String> { "error plus"s }), err.map_error([](auto x) {
        return format("{} plus", x);
    }));
}
