#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <test/test.h>
#include <unistd.h>

TEST(kill, basic) {
    static pthread_t id;
    static bool done;

    signal(SIGUSR1, [](int) {
        EXPECT_EQ(pthread_self(), id);
        done = true;
    });

    Test::TestManager::the().spawn_thread_and_block(
        [](auto thread_id) {
            id = thread_id;
            EXPECT_EQ(pthread_kill(id, SIGUSR1), 0);
        },
        [] {
            while (!done)
                asm volatile("" ::: "memory");
        });
}
