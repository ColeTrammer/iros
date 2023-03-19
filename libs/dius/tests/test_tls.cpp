#include <dius/test/prelude.h>

constinit thread_local int x = 42;

static void tls() {
    ASSERT_EQ(x, 42);
    x = 44;
    di::compiler_barrier();
    ASSERT_EQ(x, 44);
}

TEST(dius, tls)
