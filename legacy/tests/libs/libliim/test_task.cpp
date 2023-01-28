#include <liim/task.h>
#include <test/test.h>

Task<> noop_2() {
    co_return;
}

Task<> noop() {
    co_await noop_2();
    co_return;
}

Task<int> get_3() {
    co_return 3;
}

static int result = 0;

Task<> do_computation() {
    result = co_await get_3();
    co_return;
}

TEST(task, void) {
    auto task = noop();
    task();

    EXPECT(task.finished());
}

TEST(task, return_value) {
    auto task = do_computation();
    task();

    EXPECT_EQ(result, 3);
    EXPECT(task.finished());
}
