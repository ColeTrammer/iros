#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace vocab_error {
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

TESTC(vocab_error, basic)
TEST(vocab_error, erased)
}
