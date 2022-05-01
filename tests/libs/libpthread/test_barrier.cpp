#include <pthread.h>
#include <test/test.h>

TEST(barrier, basic) {
    constexpr int thread_count = 3;
    constexpr int rounds = 5;

    static pthread_barrier_t barrier;
    static int count = 0;

    EXPECT_EQ(pthread_barrier_init(&barrier, NULL, thread_count), 0);

    Test::TestManager::the().spawn_threads_and_block(thread_count, [] {
        for (size_t i = 0; i < rounds; i++) {
            if (pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
                count++;
            }
        }
    });

    EXPECT_EQ(count, rounds);

    EXPECT_EQ(pthread_barrier_destroy(&barrier), 0);
}
