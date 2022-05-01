#include <pthread.h>
#include <test/test.h>
#include <unistd.h>

static int did_cleanup;

static void do_cancel_test(Function<void()> thread_body) {
    did_cleanup = false;

    auto* result = Test::TestManager::the().spawn_thread_and_block(
        [](pthread_t id) {
            // Wait before cancelling
            usleep(500000);

            EXPECT_EQ(pthread_cancel(id), 0);
        },
        [thread_body = move(thread_body)]() {
            pthread_cleanup_push(
                [](void*) {
                    did_cleanup = 1;
                },
                nullptr);

            thread_body();

            // This shouldn't be reached.
            EXPECT_EQ(0, 1);

            pthread_cleanup_pop(0);
        });

    EXPECT_EQ((uintptr_t) result, (uintptr_t) PTHREAD_CANCELED);

    EXPECT_EQ(did_cleanup, 1);
}

// NOTE: this does not work properly in iros because
//       cancellation is not done properly. Fixing this
//       likely requires manually testing cancellation
//       before entering most system calls, but this requires
//       more thought.
#ifdef __iros__
TEST_SKIP(cancel, in_sleep) {
#else
TEST(cancel, in_sleep) {
#endif
    do_cancel_test([] {
        sleep(20);
    });
}

TEST(cancel, asynchronous) {
    do_cancel_test([] {
        EXPECT_EQ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr), 0);
        for (;;) {
            asm volatile("" ::: "memory");
        }
    });
}

TEST(cancel, after_enabling) {
    static bool did_reach;
    do_cancel_test([] {
        EXPECT_EQ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr), 0);
        usleep(600000);
        did_reach = true;
        EXPECT_EQ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr), 0);
        EXPECT_EQ(0, 1);
    });
    EXPECT_EQ(did_reach, true);
}

TEST(cancel, injected_cancelation_point) {
    do_cancel_test([] {
        for (;;) {
            pthread_testcancel();
            asm volatile("" ::: "memory");
        }
    });
}
