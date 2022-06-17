#include <ext/error.h>
#include <liim/error.h>
#include <liim/error/common_result.h>
#include <liim/error/string_domain.h>
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

using X = CommonResult<void, Result<Monostate, Ext::StringError>, Result<Monostate, int>>;
static_assert(SameAs<X, Result<Monostate, Variant<Ext::StringError, int>>>);

using Y = CommonResult<void, void, int>;
static_assert(SameAs<Y, void>);

using Z = CommonResult<int, Result<Monostate, LIIM::Error::StringError>, float, Result<Monostate, Error<>>>;
static_assert(SameAs<Z, Result<int, Error<>>>);
