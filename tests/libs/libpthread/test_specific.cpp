#include <pthread.h>
#include <test/test.h>

TEST(specific, basic) {
    constexpr int thread_count = 5;

    static pthread_key_t key;

    EXPECT_EQ(pthread_key_create(&key,
                                 [](void* v) {
                                     EXPECT_EQ(pthread_self(), (pthread_t) (uintptr_t) v);
                                 }),
              0);

    Test::TestManager::the().spawn_threads_and_block(thread_count, [] {
        EXPECT_EQ(pthread_setspecific(key, (void*) (uintptr_t) pthread_self()), 0);
        EXPECT_EQ((pthread_t) (uintptr_t) pthread_getspecific(key), pthread_self());
    });

    EXPECT_EQ(pthread_key_delete(key), 0);
}
