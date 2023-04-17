#include <di/sync/prelude.h>
#include <dius/test/prelude.h>
#include <dius/thread.h>

namespace tls {
constinit thread_local int x = 42;

static void basic() {
    ASSERT_EQ(x, 42);
    x = 44;
    di::compiler_barrier();
    ASSERT_EQ(x, 44);

    auto did_execute = di::Atomic { false };
    auto y = *dius::Thread::create(
        [&](int y) {
            ASSERT_EQ(x, 42);
            x = y;
            di::compiler_barrier();
            ASSERT_EQ(x, y);
            did_execute.store(true, di::MemoryOrder::Relaxed);
        },
        56);

    ASSERT(y.join());
    ASSERT(did_execute.load(di::MemoryOrder::Relaxed));

    di::compiler_barrier();
    ASSERT_EQ(x, 44);
}

TEST(tls, basic)
}
