#include <pthread.h>
#include <test/test.h>

static void do_cleanup_test(Function<void()> test) {
    Test::TestManager::the().spawn_thread_and_block(nullptr, move(test));
}

TEST(cleanup, basic) {
    do_cleanup_test([] {
        static int called = 0;
        pthread_cleanup_push(
            [](auto) {
                called = 1;
            },
            nullptr);
        pthread_cleanup_pop(0);

        pthread_cleanup_push(
            [](auto* x) {
                EXPECT_EQ((uintptr_t) x, (uintptr_t) &called);
                called = 1;
            },
            &called);
        pthread_cleanup_pop(1);

        EXPECT_EQ(called, 1);
    });
}

TEST(cleanup, exit) {
    static int called = 0;
    do_cleanup_test([] {
        pthread_cleanup_push(
            [](auto) {
                called = 1;
            },
            nullptr);
        pthread_exit(nullptr);
        pthread_cleanup_pop(0);
    });
    EXPECT_EQ(called, 1);
}

TEST(cleanup, nested) {
    static int x = 3;
    do_cleanup_test([] {
        pthread_cleanup_push(
            [](auto* a) {
                EXPECT_EQ((uintptr_t) a, 1u);
                EXPECT_EQ(x, 1);
                x = 0;
            },
            (void*) 1);
        pthread_cleanup_push(
            [](auto* a) {
                EXPECT_EQ((uintptr_t) a, 2u);
                EXPECT_EQ(x, 2);
                x = 1;
            },
            (void*) 2);
        pthread_cleanup_push(
            [](auto* a) {
                EXPECT_EQ((uintptr_t) a, 3u);
                EXPECT_EQ(x, 3);
                x = 2;
            },
            (void*) 3);
        pthread_exit(nullptr);
        pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);
    });
    EXPECT_EQ(x, 0);
}
