#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    di::GenericCode e = di::BasicError::OutOfRange;
    ASSERT(!e.success());
    ASSERT_EQ(e.message(), u8"Out of Range"_sv);

    ASSERT_EQ(e, di::BasicError::OutOfRange);
    ASSERT_NOT_EQ(di::BasicError::Success, e);
}

void erased() {
    di::Error e = di::BasicError::OutOfRange;
    ASSERT(!e.success());
    ASSERT_EQ(e.message(), u8"Out of Range"_sv);

    ASSERT_EQ(e, di::BasicError::OutOfRange);
    ASSERT_NOT_EQ(di::BasicError::Success, e);
}

TEST_CONSTEXPR(vocab_error, basic, basic)
TEST(vocab_error, erased) {
    erased();
}