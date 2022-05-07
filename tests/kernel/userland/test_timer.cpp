#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <test/test.h>
#include <time.h>
#include <unistd.h>

constexpr int sigev_user_value = 42;
constexpr int interval_repeat_count = 5;

static int times_fired;
static timer_t timer;
static int overrun_count = 0;

static void do_timer_test(int signo, Function<void(int)> setup, Function<void()> wait, Function<void()> cleanup) {
    times_fired = 0;
    overrun_count = 0;

    sigset_t set;
    EXPECT_EQ(sigemptyset(&set), 0);
    EXPECT_EQ(sigaddset(&set, signo), 0);
    EXPECT_EQ(sigprocmask(SIG_UNBLOCK, &set, nullptr), 0);

    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = [](int signo, siginfo_t* info, void*) {
        if (signo >= SIGRTMIN) {
            EXPECT_EQ(info->si_value.sival_int, sigev_user_value);
            overrun_count = timer_getoverrun(timer);
        }
        times_fired++;
    };
    EXPECT_EQ(sigemptyset(&act.sa_mask), 0);
    EXPECT_EQ(sigaction(signo, &act, nullptr), 0);

    setup(signo);
    wait();
    cleanup();

    signal(signo, SIG_IGN);
}

static Function<void(int)> posix_timer_setup(int clock, bool interval) {
    return [clock, interval](int signo) {
        sigevent ev;
        ev.sigev_signo = signo;
        ev.sigev_notify = SIGEV_SIGNAL;
        ev.sigev_value.sival_int = sigev_user_value;
        EXPECT_EQ(timer_create(clock, &ev, &timer), 0);

        itimerspec spec;
        spec.it_value = { .tv_sec = 0, .tv_nsec = TEST_SLEEP_SCHED_DELAY_US * 1000 };
        spec.it_interval = interval ? spec.it_value : (timespec) { .tv_sec = 0, .tv_nsec = 0 };
        EXPECT_EQ(timer_settime(timer, 0, &spec, nullptr), 0);
    };
}

static void posix_timer_cleanup() {
    EXPECT_EQ(timer_delete(timer), 0);
}

static Function<void(int)> itimer_setup(int itimer, bool interval) {
    return [itimer, interval](int) {
        itimerval val;
        val.it_value.tv_sec = 0;
        val.it_value.tv_usec = TEST_SLEEP_SCHED_DELAY_US;
        val.it_interval = interval ? val.it_value : (timeval) { .tv_sec = 0, .tv_usec = 0 };
        EXPECT_EQ(setitimer(itimer, &val, nullptr), 0);
    };
}

static Function<void()> itimer_cleanup(int itimer) {
    return [itimer] {
        itimerval val;
        memset(&val, 0, sizeof(val));
        EXPECT_EQ(setitimer(itimer, &val, nullptr), 0);
    };
}

static void singleshot_wait() {
    usleep(3 * TEST_SLEEP_SCHED_DELAY_US);
    EXPECT_EQ(times_fired, 1);
}

static void interval_wait() {
    while (times_fired < interval_repeat_count) {
        EXPECT_EQ(pause(), -1);
        EXPECT_EQ(errno, EINTR);
        asm volatile("" ::: "memory");
    }
    EXPECT_EQ(times_fired, interval_repeat_count);
}

static Function<void()> busy_poll_wait(int expected_count) {
    return [expected_count] {
        while (times_fired < expected_count) {
            asm volatile("" ::: "memory");
        }
        EXPECT_EQ(times_fired, expected_count);
    };
}

TEST(timer, posix_monotonic_singleshot) {
    do_timer_test(SIGRTMIN, posix_timer_setup(CLOCK_MONOTONIC, false), singleshot_wait, posix_timer_cleanup);
}

TEST(timer, posix_realtime_singleshot) {
    do_timer_test(SIGRTMIN, posix_timer_setup(CLOCK_REALTIME, false), singleshot_wait, posix_timer_cleanup);
}

TEST(timer, posix_monotonic_interval) {
    do_timer_test(SIGRTMIN, posix_timer_setup(CLOCK_MONOTONIC, true), interval_wait, posix_timer_cleanup);
}

TEST(timer, posix_realtime_interval) {
    do_timer_test(SIGRTMIN, posix_timer_setup(CLOCK_REALTIME, true), interval_wait, posix_timer_cleanup);
}

TEST(timer, posix_getoverrun) {
    do_timer_test(
        SIGRTMIN, posix_timer_setup(CLOCK_MONOTONIC, true),
        [] {
            sigset_t set;
            EXPECT_EQ(sigemptyset(&set), 0);
            EXPECT_EQ(sigaddset(&set, SIGRTMIN), 0);
            EXPECT_EQ(sigprocmask(SIG_BLOCK, &set, nullptr), 0);
            usleep(4 * TEST_SLEEP_SCHED_DELAY_US);
            EXPECT_EQ(sigprocmask(SIG_UNBLOCK, &set, nullptr), 0);
            EXPECT_EQ(times_fired, 1);
            EXPECT_EQ(overrun_count, 3);
        },
        posix_timer_cleanup);
}

TEST(timer, itimer_real_singleshot) {
    do_timer_test(SIGALRM, itimer_setup(ITIMER_REAL, false), singleshot_wait, itimer_cleanup(ITIMER_REAL));
}

TEST(timer, itimer_real_interval) {
    do_timer_test(SIGALRM, itimer_setup(ITIMER_REAL, true), interval_wait, itimer_cleanup(ITIMER_REAL));
}

TEST(timer, itimer_profile_singleshot) {
    do_timer_test(SIGPROF, itimer_setup(ITIMER_PROF, false), busy_poll_wait(1), itimer_cleanup(ITIMER_PROF));
}

TEST(timer, itimer_profile_interval) {
    do_timer_test(SIGPROF, itimer_setup(ITIMER_PROF, true), busy_poll_wait(interval_repeat_count), itimer_cleanup(ITIMER_PROF));
}

TEST(timer, itimer_virtual_singleshot) {
    do_timer_test(SIGVTALRM, itimer_setup(ITIMER_VIRTUAL, false), busy_poll_wait(1), itimer_cleanup(ITIMER_VIRTUAL));
}

TEST(timer, itimer_virtual_interval) {
    do_timer_test(SIGVTALRM, itimer_setup(ITIMER_VIRTUAL, true), busy_poll_wait(interval_repeat_count), itimer_cleanup(ITIMER_VIRTUAL));
}
