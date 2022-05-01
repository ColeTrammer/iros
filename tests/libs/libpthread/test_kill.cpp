#include <pthread.h>
#include <signal.h>
#include <test/test.h>
#include <unistd.h>

TEST(kill, basic) {
    static pthread_t id;

    signal(SIGUSR1, [](int) {
        EXPECT_EQ(pthread_self(), id);
        pthread_exit(nullptr);
    });

    Test::TestManager::the().spawn_thread_and_block(
        [](auto thread_id) {
            id = thread_id;
            EXPECT_EQ(pthread_kill(id, SIGUSR1), 0);
        },
        [] {
            sleep(10);
        });
}
