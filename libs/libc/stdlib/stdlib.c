#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

long long llabs(long long n) {
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

lldiv_t lldiv(long long a, long long b) {
    return (lldiv_t) { a / b, a % b };
}

#ifndef __is_libk
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
        char *const args[] = { "sh", "-c", (char *) cmd, NULL };

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

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

int posix_openpt(int flags) {
    return open("/dev/ptmx", flags);
}

static char pts_name_buf[24];

char *ptsname(int fd) {
    if (ptsname_r(fd, pts_name_buf, 24) == 0) {
        return pts_name_buf;
    }

    return NULL;
}

int ptsname_r(int fd, char *buf, size_t buflen) {
    int pts_num;
    int ret = ioctl(fd, TGETNUM, &pts_num);
    if (ret != 0) {
        return ret;
    }

    bool worked = snprintf(buf, buflen, "/dev/tty%d", pts_num) < (int) buflen;
    return worked ? 0 : -1;
}

// We don't support anything other than plain ascii
int mblen(const char *s, size_t n) {
    (void) s;
    (void) n;

    return 1;
}

static char *mktemp_impl(char *s, bool care_about_invalid_names) {
    size_t len = strlen(s);
    if (len < 6) {
        errno = EINVAL;
        return s;
    }

    size_t start = len - 6;
    bool first_attempt = false;
    unsigned int seed = time(NULL);

    for (; start < len; start++) {
        if (care_about_invalid_names && first_attempt && s[start] != 'X') {
            errno = EINVAL;
            memset(s + start, 'X', 6);
            return s;
        }

        const char *possible_values = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        s[start] = possible_values[rand_r(&seed) % 52];
    }

    return s;
}

char *mktemp(char *s) {
    return mktemp_impl(s, true);
}

int mkstemp(char *t) {
    errno = 0;
    for (int i = 0; i < 100; i++) {
        mktemp_impl(t, i == 0);
        if (errno != 0) {
            return -1;
        }

        int fd = open(t, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd == -1) {
            continue;
        }

        return fd;
    }

    errno = EEXIST;
    return -1;
}

char *mkdtemp(char *t) {
    errno = 0;
    for (int i = 0; i < 100; i++) {
        mktemp_impl(t, i == 0);
        if (errno != 0) {
            return NULL;
        }

        int ret = mkdir(t, 0700);
        if (ret) {
            continue;
        }

        return t;
    }

    errno = EEXIST;
    return NULL;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    void *ptr = aligned_alloc(alignment, size);
    if (ptr == NULL) {
        errno = ENOMEM;
        return -1;
    }

    *memptr = ptr;
    return 0;
}

#endif /* __is_libk */
