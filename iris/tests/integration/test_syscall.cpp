#include <di/math/prelude.h>
#include <dius/system/system_call.h>
#include <dius/test/prelude.h>

static void allocate_memory() {
    auto x = dius::system::system_call<uptr>(dius::system::Number::allocate_memory, 4096);
    ASSERT(x);

    auto y =
        dius::system::system_call<uptr>(dius::system::Number::allocate_memory, 16_u64 * 1024_u64 * 1024_u64 * 1024_u64);
    ASSERT_EQ(y, di::Unexpected(di::BasicError::NotEnoughMemory));
}

static void fault() {
    // Invalid userspace memory address
    auto x = dius::system::system_call<usize>(dius::system::Number::write, 1, 0xCAFEBEBE, 1024);
    ASSERT_EQ(x, di::Unexpected(di::BasicError::BadAddress));

    // Kernel memory address
    auto y = dius::system::system_call<usize>(dius::system::Number::write, 1, 0xFFFFFFFF80000000, 1024);
    ASSERT_EQ(y, di::Unexpected(di::BasicError::BadAddress));

    // Non-canonical address
    auto z = dius::system::system_call<usize>(dius::system::Number::write, 1, 0xF000000080000000, 1024);
    ASSERT_EQ(z, di::Unexpected(di::BasicError::BadAddress));

    // Overflow for size.
    auto w = dius::system::system_call<usize>(dius::system::Number::write, 1, di::to_uintptr(&fault),
                                              di::NumericLimits<usize>::max);
    ASSERT_EQ(w, di::Unexpected(di::BasicError::BadAddress));
}

TEST(syscall, allocate_memory)
TEST(syscall, fault)
