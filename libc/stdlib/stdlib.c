#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>

#ifndef __is_libk

static unsigned int seed;

void srand(unsigned int _seed) {
    seed = _seed;
}

int rand(void) {
    return rand_r(&seed);
}

// Use LCG method with same parameters as glibc
int rand_r(unsigned int *seedp) {
    *seedp = *seedp * 1103515245 + 12345;
    return (int) (*seedp % RAND_MAX);
}

#endif /* __is_libk */

int abs(int n) {
    if (n < 0) { 
        return -n; 
    }

    return n;
}

long labs(long n) {
    if (n < 0) {
        return -n;
    }

    return n;
}

div_t div(int a, int b) {
    return (div_t) { a / b, a % b };
}

ldiv_t ldiv(long a, long b) {
    return (ldiv_t) { a / b, a % b };
}

int system(const char *cmd) {
    // We have to check whether or not /bin/sh exists, but it should always exist so just return true
    if (cmd == NULL) {
        return true;
    }

    struct sigaction int_save, quit_save;
    struct sigaction ign;

    ign.sa_flags = 0;
    sigemptyset(&ign.sa_mask);
    sigaddset(&ign.sa_mask, SIGCHLD);
    ign.sa_handler = SIG_IGN;

    sigset_t block_save;
    sigprocmask(SIG_BLOCK, &ign.sa_mask, &block_save);

    sigaction(SIGINT, &ign, &int_save);
    sigaction(SIGQUIT, &ign, &quit_save);

    pid_t pid = fork();
    if (pid == 0) {
        sigaction(SIGINT, &int_save, NULL);
        sigaction(SIGQUIT, &quit_save, NULL);
        sigprocmask(SIG_SETMASK, &block_save, NULL);

        // Could use execl instead
        char *const args[] = {
            "sh", "-c", (char*) cmd, NULL
        };

        execve("/bin/sh", args, environ);

        // This should never happen
        _exit(127);
    }

    int status = 0;
    if (pid == -1) {
        status = -1;
    } else {
        // Wait for created process
        while (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
    }

    sigaction(SIGINT, &int_save, NULL);
    sigaction(SIGQUIT, &quit_save, NULL);
    sigprocmask(SIG_SETMASK, &block_save, NULL);
    return status;
}