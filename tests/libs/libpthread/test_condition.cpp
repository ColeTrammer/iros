#include <pthread.h>
#include <test/test.h>
#include <unistd.h>

TEST(condition, basic) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    static bool signalled = false;

    Test::TestManager::the().spawn_thread_and_block(
        [](auto) {
            usleep(500000);

            Test::TestManager::the().spawn_thread_and_block(nullptr, [] {
                EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
                EXPECT_EQ(pthread_cond_signal(&cond), 0);
                signalled = true;
                (void) signalled;
                EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);
            });
        },
        [] {
            EXPECT_EQ(pthread_mutex_lock(&mutex), 0);
            EXPECT_EQ(pthread_cond_wait(&cond, &mutex), 0);
            EXPECT_EQ(signalled, true);
            EXPECT_EQ(pthread_mutex_unlock(&mutex), 0);
        });

    EXPECT_EQ(pthread_cond_destroy(&cond), 0);
}
