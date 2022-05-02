#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <test/test.h>
#include <unistd.h>

#ifndef BINARY_DIR
#define BINARY_DIR "."
#endif

TEST(spawn, basic) {
    posix_spawn_file_actions_t acts;
    EXPECT_EQ(posix_spawn_file_actions_init(&acts), 0);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    int fds[2];
    EXPECT_EQ(pipe(fds), 0);

    EXPECT_EQ(posix_spawn_file_actions_addclose(&acts, fds[0]), 0);
    EXPECT_EQ(posix_spawn_file_actions_addopen(&acts, 11, "/dev/full", O_RDWR, 0), 0);
    EXPECT_EQ(posix_spawn_file_actions_adddup2(&acts, 11, 0), 0);
    EXPECT_EQ(posix_spawn_file_actions_addclose(&acts, 11), 0);
    EXPECT_EQ(posix_spawn_file_actions_adddup2(&acts, fds[1], 4), 0);

    EXPECT_EQ(posix_spawn_file_actions_addclose(&acts, -1), EBADF);

    signal(SIGWINCH, SIG_IGN);

    sigset_t set;
    EXPECT_EQ(sigemptyset(&set), 0);
    EXPECT_EQ(sigaddset(&set, SIGWINCH), 0);

    EXPECT_EQ(posix_spawnattr_setsigdefault(&attr, &set), 0);
    EXPECT_EQ(posix_spawnattr_setsigmask(&attr, &set), 0);

    EXPECT_EQ(posix_spawnattr_setpgroup(&attr, 0), 0);
    EXPECT_EQ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF), 0);

    char* const args[] = { (char*) BINARY_DIR "/test_spawn_basic_helper", NULL };

    pid_t pid;
    EXPECT_EQ(posix_spawn(&pid, args[0], &acts, &attr, args, nullptr), 0);

    EXPECT_EQ(close(fds[1]), 0);

    char buf[1];
    EXPECT_EQ(read(fds[0], buf, sizeof(buf)), 1);

    EXPECT_EQ(buf[0], 'x');

    int status;
    EXPECT_EQ(waitpid(pid, &status, 0), pid);
    EXPECT(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);

    EXPECT_EQ(posix_spawn_file_actions_destroy(&acts), 0);
    EXPECT_EQ(posix_spawnattr_destroy(&attr), 0);
}

#ifdef SPAWN_BASIC_HELPER
int main() {
    sigset_t set;
    sigprocmask(0, NULL, &set);

    assert(sigismember(&set, SIGWINCH));
    assert(signal(SIGWINCH, SIG_IGN) == SIG_DFL);

    char c = 'x';
    assert(write(0, &c, 1) < 0);
    assert(errno == ENOSPC);

    assert(read(11, &c, 1) < 0);
    assert(errno == EBADF);

    assert(getpid() == getpgid(0));
    assert(write(4, &c, 1) == 1);

    return 0;
}
#endif
