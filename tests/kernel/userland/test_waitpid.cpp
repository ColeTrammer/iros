#include <errno.h>
#include <sys/wait.h>
#include <test/test.h>
#include <unistd.h>

#ifndef BINARY_DIR
#define BINARY_DIR "."
#endif

constexpr int child_status = 42;
constexpr int runner_status = 68;

static void do_waitpid_test(Function<void(pid_t)> before_wait, Function<void()> in_child, Function<void(pid_t)> custom_waiter = nullptr) {
    pid_t runner = fork();
    EXPECT(runner >= 0);
    if (runner > 0) {
        int status;
        EXPECT_EQ(waitpid(runner, &status, 0), runner);
        EXPECT(WIFEXITED(status));
        EXPECT_EQ(WEXITSTATUS(status), runner_status);
        return;
    }

    auto child = fork();
    EXPECT(child >= 0);

    if (child == 0) {
        in_child.safe_call();
        _exit(child_status);
    }

    before_wait.safe_call(child);

    if (custom_waiter) {
        custom_waiter(child);
    } else {
        int status;
        auto result = waitpid(child, &status, 0);
        EXPECT_EQ(result, child);
        EXPECT(WIFEXITED(status));
        EXPECT_EQ(WEXITSTATUS(status), child_status);
        EXPECT_EQ(waitpid(-1, &status, 0), -1);
        EXPECT_EQ(errno, ECHILD);
    }

    _exit(68);
}

TEST(waitpid, basic) {
    do_waitpid_test(nullptr, nullptr);
}

TEST(waitpid, exit_before_waitpid) {
    do_waitpid_test(
        [](auto) {
            usleep(TEST_SLEEP_SCHED_DELAY_US);
        },
        nullptr);
}

TEST(waitpid, exit_after_waitpid) {
    do_waitpid_test(nullptr, [] {
        usleep(TEST_SLEEP_SCHED_DELAY_US);
    });
}

TEST(waitpid, wnohang) {
    do_waitpid_test(
        [](auto child) {
            EXPECT_EQ(waitpid(child, nullptr, WNOHANG), 0);
        },
        [] {
            usleep(TEST_SLEEP_SCHED_DELAY_US);
        });
}

TEST(waitpid, stop_continue) {
    do_waitpid_test(
        [](auto child) {
            int status;
            EXPECT_EQ(kill(child, SIGSTOP), 0);
            EXPECT_EQ(waitpid(child, &status, WUNTRACED | WCONTINUED), child);
            EXPECT(WIFSTOPPED(status));
            EXPECT_EQ(WSTOPSIG(status), SIGSTOP);
            EXPECT_EQ(kill(child, SIGCONT), 0);
            EXPECT_EQ(waitpid(child, &status, WUNTRACED | WCONTINUED), child);
            EXPECT(WIFCONTINUED(status));
        },
        [] {
            usleep(TEST_SLEEP_SCHED_DELAY_US);
        });
}

TEST(waitpid, signalled) {
    do_waitpid_test(
        [](auto child) {
            EXPECT_EQ(kill(child, SIGKILL), 0);
        },
        [] {
            pause();
        },
        [](auto child) {
            int status;
            EXPECT_EQ(waitpid(child, &status, 0), child);
            EXPECT(WIFSIGNALED(status));
            EXPECT_EQ(WTERMSIG(status), SIGKILL);
        });
}

TEST(waitpid, execve) {
    do_waitpid_test(
        [](auto) {
            execl(BINARY_DIR "/waitpid_exec_helper", "waitpid_exec_helper", nullptr);
        },
        nullptr, nullptr);
}

#ifdef WAITPID_EXEC_HELPER
int main() {
    int status;
    assert(waitpid(-1, &status, 0));
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == child_status);
    return runner_status;
}
#endif
