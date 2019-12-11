#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H 1

#include <bits/timeval.h>
#include <signal.h>
#include <time.h>

#define FD_SETSIZE 16

#define FD_CLR(fd, set)   (void)
#define FD_ISSET(fd, set) (void)
#define FD_SET(fd, set)   (void)
#define FD_ZERO(set)      (void)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    unsigned short fds;
} fd_set;

int pselect(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
            const struct timespec *__restrict timeout, const sigset_t *__restrict sigmask);
int select(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
           const struct timespec *__restrict timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SELECT_H */