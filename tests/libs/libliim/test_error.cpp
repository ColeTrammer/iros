#include <ext/error.h>
#include <liim/error.h>
#include <liim/error/common_result.h>
#include <liim/error/string_domain.h>
#include <liim/error/system_domain.h>
#include <liim/error/typed_domain.h>
#include <test/test.h>

TEST(error, typed) {
    auto s = make_string_error("{}", "hello");
    EXPECT_EQ(s.message(), "hello");
}

TEST(error, erased) {
    Error<> e = make_string_error("{}", "hello");
    EXPECT_EQ(e.message(), "hello");
}

TEST(error, system) {
    Error<> e = make_system_error(EBADF);
    EXPECT_EQ(e.message(), strerror(EBADF));
}

using Type = struct {};

using X = CommonResult<void, Result<void, Type>, Result<void, int>>;
static_assert(SameAs<X, Result<void, Variant<Type, int>>>);

using Y = CommonResult<void, void, int>;
static_assert(SameAs<Y, void>);

using Z = CommonResult<int, Result<void, StringError>, float, Result<void, Error<>>>;
static_assert(SameAs<Z, Result<int, Error<>>>);
