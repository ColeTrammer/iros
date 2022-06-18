#include <liim/construct.h>
#include <liim/result.h>
#include <liim/string.h>
#include <test/test.h>

TEST(result, basic_getters) {
    auto ok = Result<int, String> { 5 };
    auto err = Result<int, String> { Err("hello"s) };

    EXPECT(ok);
    EXPECT(!!ok);
    EXPECT(!err);

    EXPECT_EQ(ok.value(), 5);
    EXPECT_EQ(err.error(), "hello"s);

    EXPECT_EQ(ok, ok);
}

TEST(result, functional) {
    auto make_ok = [] {
        return Result<int, String> { 5 };
    };
    auto make_err = [] {
        return Result<int, String> { Err("error"s) };
    };

    EXPECT_EQ((Result<int, String> { 10 }), make_ok().transform([](auto x) {
        return x * 2;
    }));

    EXPECT_EQ((Result<int, String> { Err("error"s) }), make_err().transform([](auto x) {
        return x * 2;
    }));

    EXPECT_EQ((Result<int, String> { 5 }), make_ok().transform_error([](auto x) {
        return format("{} plus", x);
    }));

    EXPECT_EQ((Result<int, String> { Err("error plus"s) }), make_err().transform_error([](auto x) {
        return format("{} plus", x);
    }));
}

TEST(result, conversion) {
    char c = 5;
    auto x = Result<char, Monostate> { c };
    Result<int, Monostate> y = move(x);
    EXPECT_EQ(y.value(), 5);
}

constexpr void construct() {
    struct X {
        constexpr static Result<X, StringView> create(int x) {
            if (x != 0) {
                return Err("fail"sv);
            }
            return X {};
        }
    };

    EXPECT(X::create(42).is_error());
    EXPECT(create<X>(42).is_error());

    int z = 5;
    auto y = Result<int, StringView> { Err("fail"sv) };
    auto x = tuple_result_sequence(z, y);
    EXPECT_EQ(x.error(), "fail"sv);

    auto a = create<X>(Result<int, int>(Err(42)));
    EXPECT_EQ(a.error().as<int>(), 42);

    auto b = create<X>(Result<int, int>(0));
    EXPECT(b);
}

constexpr void result_and_then_test() {
    EXPECT_EQ(5, result_and_then(5, [](auto x) {
                  return x;
              }));

    EXPECT_EQ((Result<int, int>(2)), result_and_then(Result<int, int>(2), [](auto x) {
                  return x;
              }));

    EXPECT_EQ((Result<int, StringView>(Err("xxx"sv))), result_and_then(Result<int, StringView>(2), [](auto) {
                  return Result<int, StringView>(Err("xxx"sv));
              }));
}

TEST_CONSTEXPR(result, construct, construct)
TEST_CONSTEXPR(result, result_and_then_test, result_and_then_test)
