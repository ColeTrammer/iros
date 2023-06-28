#include <di/vocab/error/error.h>
#include <di/vocab/error/meta/common_error.h>
#include <di/vocab/error/prelude.h>
#include <dius/test/prelude.h>

namespace vocab_error {
constexpr void basic() {
    di::GenericCode e = di::BasicError::ResultOutOfRange;
    ASSERT(!e.success());
    ASSERT_EQ(e.message(), u8"Result out of range"_sv);

    ASSERT_EQ(e, di::BasicError::ResultOutOfRange);
    ASSERT_NOT_EQ(di::BasicError::Success, e);
}

void erased() {
    di::Error e = di::BasicError::ResultOutOfRange;
    ASSERT(!e.success());
    ASSERT_EQ(e.message(), u8"Result out of range"_sv);

    ASSERT_EQ(e, di::BasicError::ResultOutOfRange);
    ASSERT_NOT_EQ(di::BasicError::Success, e);
}

constexpr void common_error() {
    using W = int;
    using X = di::Error;
    using Y = di::BasicError;
    using Z = di::GenericCode;

    static_assert(di::SameAs<di::meta::CommonError<X, Y, Z>, di::Error>);
    static_assert(di::SameAs<di::meta::CommonError<Y, Z>, di::GenericCode>);
    static_assert(di::SameAs<di::meta::CommonError<X, Y>, di::Error>);
    static_assert(di::SameAs<di::meta::CommonError<W, X, Y, Z>, di::Variant<W, X, Y, Z>>);
    static_assert(di::SameAs<di::meta::CommonError<X, Y, Z, W>, di::Variant<X, W>>);

    static_assert(di::concepts::CommonErrorWith<X, Y>);
    static_assert(di::concepts::CommonErrorWith<X, Z>);
}

TESTC(vocab_error, basic)
TEST(vocab_error, erased)
TESTC(vocab_error, common_error)
}
