#include <errno.h>
#include <pthread.h>
#include <test/test.h>
#include <unistd.h>

TEST(detached, basic) {
    static bool ran = false;

    pthread_t id;
    pthread_attr_t attr;
    EXPECT_EQ(pthread_attr_init(&attr), 0);
    EXPECT_EQ(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED), 0);

    EXPECT_EQ(pthread_create(
                  &id, &attr,
                  [](void*) -> void* {
                      ran = true;
                      return nullptr;
                  },
                  nullptr),
              0);

    EXPECT_EQ(pthread_join(id, nullptr), EINVAL);

    usleep(TEST_SLEEP_SCHED_DELAY_US);

    EXPECT_EQ(ran, true);
}
