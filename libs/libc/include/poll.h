#ifndef _POLL_H
#define _POLL_H 1

#define POLLIN     1
#define POLLRDNORM 2
#define POLLRDBAND 4
#define POLLPRI    8
#define POLLOUT    16
#define POLLWRNORM 32
#define POLLWRBAND 64
#define POLLERR    128
#define POLLHUP    256
#define POLLNVAL   512

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct pollfd {
    int fd;
    short events;
    short revents;
};

typedef unsigned short nfds_t;

int poll(struct pollfd[], nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _POLL_H */
