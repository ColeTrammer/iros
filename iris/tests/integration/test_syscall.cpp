#include <dius/test/prelude.h>

static void allocate_memory() {
    auto x = dius::system::system_call<uptr>(dius::system::Number::allocate_memory, 4096);
    ASSERT(x);

    auto y =
        dius::system::system_call<uptr>(dius::system::Number::allocate_memory, 16_u64 * 1024_u64 * 1024_u64 * 1024_u64);
    ASSERT_EQ(y, di::Unexpected(dius::PosixError::NotEnoughMemory));
}

TEST(syscall, allocate_memory)
