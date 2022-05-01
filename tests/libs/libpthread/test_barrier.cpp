#include <pthread.h>
#include <test/test.h>

TEST(barrier, basic) {
    constexpr int thread_count = 3;
    constexpr int rounds = 5;

    static pthread_barrier_t barrier;
    static int count = 0;

    EXPECT_EQ(pthread_barrier_init(&barrier, NULL, thread_count), 0);

    pthread_t threads[thread_count];
    for (size_t i = 0; i < thread_count; i++) {
        EXPECT_EQ(pthread_create(
                      &threads[i], nullptr,
                      [](void*) -> void* {
                          for (size_t i = 0; i < 5; i++) {
                              if (pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
                                  count++;
                              }
                          }

                          return nullptr;
                      },
                      nullptr),
                  0);
    }

    for (size_t i = 0; i < thread_count; i++) {
        EXPECT_EQ(pthread_join(threads[i], nullptr), 0);
    }

    EXPECT_EQ(count, rounds);

    EXPECT_EQ(pthread_barrier_destroy(&barrier), 0);
}
