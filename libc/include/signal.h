#ifndef _SIGNAL_H
#define _SIGNAL_H 1

#include <sys/types.h>

typedef uint32_t sig_atomic_t;

typedef void (*sighandler_t)(int);

int raise(int sig);
sighandler_t signal(int signum, sighandler_t handler);

#endif /* _SIGNAL_H */