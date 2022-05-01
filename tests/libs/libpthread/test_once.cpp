#include <pthread.h>
#include <test/test.h>

TEST(once, basic) {
    constexpr int thread_count = 5;

    static pthread_once_t once = PTHREAD_ONCE_INIT;
    static int counter = 0;

    Test::TestManager::the().spawn_threads_and_block(thread_count, [] {
        pthread_once(&once, [] {
            counter += 1;
        });
    });

    EXPECT_EQ(counter, 1);
}
