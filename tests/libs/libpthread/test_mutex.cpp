#include <errno.h>
#include <pthread.h>
#include <test/test.h>
#include <unistd.h>

TEST(mutex, basic) {
    // NOTE: adding more thread, more threads, or enabling
    //       SMP all make this test flaky on iros. SMP is
    //       prone to outright crash, but other configurations
    //       may deadlock.
    constexpr int thread_count = 4;
    constexpr int iterations_per_thread = 100000;

    static pthread_mutex_t mutex;
    static int counter;

    EXPECT_EQ(pthread_mutex_init(&mutex, nullptr), 0);

    Test::TestManager::the().spawn_threads_and_block(thread_count, [] {
        for (int i = 0; i < iterations_per_thread; i++) {
            EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
            int c = *(volatile int*) &counter;
            *(volatile int*) &counter = c + 1;
            EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);
        }
    });

    EXPECT_EQ(pthread_mutex_destroy(&mutex), 0);

    EXPECT_EQ(counter, thread_count * iterations_per_thread);
}

TEST(mutex, robust) {
    static pthread_mutexattr_t attr;
    static pthread_mutex_t mutex;
    EXPECT_EQ(pthread_mutexattr_init(&attr), 0);
    EXPECT_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL), 0);
    EXPECT_EQ(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST), 0);
    EXPECT_EQ(pthread_mutex_init(&mutex, &attr), 0);

    Test::TestManager::the().spawn_thread_and_block(
        [](auto) {
            usleep(500000);

            EXPECT_EQ(pthread_mutex_lock(&mutex), EOWNERDEAD);
            EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);

            EXPECT_EQ(pthread_mutex_lock(&mutex), ENOTRECOVERABLE);
            EXPECT_EQ(pthread_mutex_unlock(&mutex), EPERM);
        },
        [] {
            EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
            pthread_exit(nullptr);
        });
}

TEST(mutex, robust_consistent) {
    static pthread_mutexattr_t attr;
    static pthread_mutex_t mutex;
    EXPECT_EQ(pthread_mutexattr_init(&attr), 0);
    EXPECT_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL), 0);
    EXPECT_EQ(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST), 0);
    EXPECT_EQ(pthread_mutex_init(&mutex, &attr), 0);

    Test::TestManager::the().spawn_thread_and_block(
        [](auto) {
            usleep(500000);

            EXPECT_EQ(pthread_mutex_lock(&mutex), EOWNERDEAD);
            EXPECT_EQ(pthread_mutex_consistent(&mutex), 0);
            EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);

            EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
            EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);
        },
        [] {
            EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
            pthread_exit(nullptr);
        });
}
