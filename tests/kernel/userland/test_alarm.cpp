#include <errno.h>
#include <signal.h>
#include <test/test.h>
#include <unistd.h>

#ifndef BINARY_DIR
#define BINARY_DIR "."
#endif

TEST(alarm, basic) {
    static sig_atomic_t did_get_alarm = 0;
    signal(SIGALRM, [](int) {
        did_get_alarm = 1;
    });

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGINT);

    EXPECT(!did_get_alarm);
    ualarm(TEST_SLEEP_SCHED_DELAY_US, 0);
    EXPECT(sigsuspend(&set) == -1);
    EXPECT_EQ(errno, EINTR);
    EXPECT(did_get_alarm);
    ualarm(0, 0);
}

TEST(alarm, cancel) {
    static sig_atomic_t did_get_alarm = 0;
    signal(SIGALRM, [](int) {
        did_get_alarm = 1;
    });

    EXPECT(!did_get_alarm);
    ualarm(TEST_SLEEP_SCHED_DELAY_US, 0);
    ualarm(0, 0);
    usleep(TEST_SLEEP_SCHED_DELAY_US + 500);
    EXPECT(!did_get_alarm);
}

TEST(alarm, exec) {
    sigset_t set;
    EXPECT_EQ(sigemptyset(&set), 0);
    EXPECT_EQ(sigaddset(&set, SIGALRM), 0);
    EXPECT_EQ(sigprocmask(SIG_BLOCK, &set, nullptr), 0);

    auto exit_status = Test::TestManager::the().spawn_process_and_block(
        [] {
            ualarm(TEST_SLEEP_SCHED_DELAY_US, 0);
        },
        BINARY_DIR "/test_alarm_exec_helper");
    EXPECT_EQ(exit_status, 0);
}

#ifdef ALARM_EXEC_HELPER
int main() {
    static sig_atomic_t did_get_alarm = 0;
    signal(SIGALRM, [](int) {
        did_get_alarm = 1;
    });

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGINT);

    sigsuspend(&set);
    return did_get_alarm ? 0 : 1;
}
#endif
