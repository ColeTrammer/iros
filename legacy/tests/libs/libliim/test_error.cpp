#include <ext/error.h>
#include <liim/error.h>
#include <liim/error/common_result.h>
#include <liim/error/string_domain.h>
#include <liim/error/system_domain.h>
#include <liim/error/typed_domain.h>
#include <test/test.h>

TEST(error, typed) {
    auto s = make_string_error("{}", "hello");
    EXPECT_EQ(s.message(), "hello"sv);
}

TEST(error, erased) {
    Error<> e = make_string_error("{}", "hello");
    EXPECT_EQ(e.message(), "hello"sv);
}

TEST(error, system) {
    Error<> e = make_system_error(EBADF);
    EXPECT_EQ(e.message(), StringView(strerror(EBADF)));
}

struct XError {
private:
    friend Error<> tag_invoke(Tag<into_erased_error>, XError&&) { return make_string_error("XError"); }
};

TEST(error, into_erased) {
    Error<> e = XError {};
    EXPECT_EQ(e.message(), "XError"sv);
}

using Type = struct {};

using X = CommonResult<void, Result<void, Type>, Result<void, int>>;
static_assert(SameAs<X, Result<void, Variant<Type, int>>>);

using Y = CommonResult<void, void, int>;
static_assert(SameAs<Y, void>);

using Z = CommonResult<int, Result<void, StringError>, float, Result<void, Error<>>>;
static_assert(SameAs<Z, Result<int, Error<>>>);
