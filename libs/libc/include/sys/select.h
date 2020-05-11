#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H 1

#include <bits/timeval.h>
#include <signal.h>
#include <time.h>

#define FD_SETSIZE 16

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    unsigned char fds[FD_SETSIZE / sizeof(unsigned char) / __CHAR_BIT__];
} fd_set;

int pselect(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
            const struct timespec *__restrict timeout, const sigset_t *__restrict sigmask);
int select(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
           const struct timespec *__restrict timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define FD_CLR(fd, set) (set)->fds[fd / sizeof(unsigned char) / __CHAR_BIT__] &= ~(1U << (fd % (sizeof(unsigned char) * __CHAR_BIT__)))

#define FD_SET(fd, set) (set)->fds[fd / sizeof(unsigned char) / __CHAR_BIT__] |= (1U << (fd % (sizeof(unsigned char) * __CHAR_BIT__)))

#define FD_ISSET(fd, set) ((set)->fds[fd / sizeof(unsigned char) / __CHAR_BIT__] & (1U << (fd % (sizeof(unsigned char) * __CHAR_BIT__))))

#define FD_ZERO(set)                                                                            \
    do {                                                                                        \
        for (unsigned long i = 0; i < FD_SETSIZE / sizeof(unsigned char) / __CHAR_BIT__; i++) { \
            (set)->fds[i] = 0;                                                                  \
        }                                                                                       \
    } while (0)

#endif /* _SYS_SELECT_H */
