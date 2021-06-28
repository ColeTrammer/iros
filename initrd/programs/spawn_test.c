#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int do_checks() {
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

int main(int argc, char** argv) {
    (void) argc;

    char* test = getenv("__DID_EXEC__");
    if (test) {
        return do_checks();
    }

    posix_spawn_file_actions_t acts;
    posix_spawn_file_actions_init(&acts);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    int fds[2];
    if (pipe(fds) < 0) {
        perror("spawn_test: pipe");
        return 1;
    }

    posix_spawn_file_actions_addclose(&acts, fds[0]);
    posix_spawn_file_actions_addopen(&acts, 11, "/dev/full", O_RDWR, 0);
    posix_spawn_file_actions_adddup2(&acts, 11, 0);
    posix_spawn_file_actions_addclose(&acts, 11);
    posix_spawn_file_actions_adddup2(&acts, fds[1], 4);

    assert(posix_spawn_file_actions_addclose(&acts, -1) == EBADF);

    signal(SIGWINCH, SIG_IGN);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGWINCH);

    posix_spawnattr_setsigdefault(&attr, &set);
    posix_spawnattr_setsigmask(&attr, &set);

    posix_spawnattr_setpgroup(&attr, 0);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);

    char* const args[] = { *argv, NULL };
    char* const env[] = { "__DID_EXEC__=1", NULL };

    pid_t pid;
    posix_spawnp(&pid, *argv, &acts, &attr, args, env);

    close(fds[1]);

    char buf[1];
    if (read(fds[0], buf, sizeof(buf)) < 0) {
        perror("spawn_test: read");
        return 1;
    }

    if (waitpid(pid, NULL, 0) < 0) {
        perror("spawn_test: waitpid");
        return 1;
    }

    posix_spawn_file_actions_destroy(&acts);
    posix_spawnattr_destroy(&attr);

    return 0;
}
