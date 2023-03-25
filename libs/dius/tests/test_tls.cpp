#include <dius/test/prelude.h>

constinit thread_local int x = 42;

static void tls() {
    ASSERT_EQ(x, 42);
    x = 44;
    di::compiler_barrier();
    ASSERT_EQ(x, 44);

#ifndef DIUS_USE_RUNTIME
    auto did_execute = false;
    auto y = *dius::Thread::create(
        [&](int y) {
            ASSERT_EQ(x, 42);
            x = y;
            di::compiler_barrier();
            ASSERT_EQ(x, y);
            did_execute = true;
        },
        56);

    ASSERT(y.join());
    ASSERT(did_execute);

    di::compiler_barrier();
    ASSERT_EQ(x, 44);
#endif
}

TEST(dius, tls)
