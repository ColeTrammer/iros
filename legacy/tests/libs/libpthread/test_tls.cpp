#include <pthread.h>
#include <test/test.h>

TEST(tls, basic) {
    static __thread int x;

    x = 3;
    asm volatile("" ::: "memory");

    EXPECT_EQ(x, 3);

    Test::TestManager::the().spawn_thread_and_block(nullptr, [] {
        x = 4;
        asm volatile("" ::: "memory");
        EXPECT_EQ(x, 4);
    });

    asm volatile("" ::: "memory");
    EXPECT_EQ(x, 3);
}
