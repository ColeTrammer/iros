#ifndef _SIGNAL_H
#define _SIGNAL_H 1

#include <sys/types.h>
#include <time.h>

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3

#define SIGTRAP 5
#define SIGABRT 6


#define SIGKILL 9
#define SIGTTIN 10
#define SIGTTOU 11


#define SIGALRM 14
#define SIGTERM 15
#define NUM_SIGNALS 32

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// Not sure what this is...
typedef int sig_atomic_t;

// Again IDK
typedef unsigned int sigset_t;

typedef void (*sighandler_t)(int);

union sigval {
    int sival_int;
    void *sival_ptr;
};

struct sigevent {
    int sigev_notify;
    int sigev_signo;
    union sigval sigev_value;
    void (*sigev_notify_function)(union sigval);
    pthread_attr_t *sigev_notify_attributes;
};

typedef struct {
    void *ss_sp;
    size_t ss_size;
    int ss_flags;
} stack_t;

typedef struct {
    int si_signo;
    int si_code;
    int si_errno;
    pid_t si_pid;
    uid_t si_uid;
    void *si_addr;
    int si_status;
    long si_band;
    union sigval si_value;
} siginfo_t;

struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_sigaction)(int, siginfo_t*, void*);
};

int kill(pid_t pid, int num);
int killpg(pid_t pid, int num);
void psiginfo(const siginfo_t *info, const char *s);
void psignal(int n, const char *s);
int pthread_kill(pthread_t thread, int n);
int pthread_sigmask(int how, const sigset_t *__restrict set, sigset_t *__restrict old);
int raise(int sig);

int sigaction(int num, const struct sigaction *act, struct sigaction *oldact);
int sigaddset(sigset_t *set, int num);
int sigaltstack(const stack_t *__restrict ss, stack_t *__restrict old);
int sigdelset(sigset_t *set, int num);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sighold(int n);
int sigignore(int n);
int siginterrupt(int n, int flag);
int sigismember(const sigset_t *set, int n);
sighandler_t signal(int signum, sighandler_t handler);
int sigpause(int n);
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *old);
int sigqueue(pid_t pid, int n, union sigval);
int sigrelse(int n);
sighandler_t sigset(int n, sighandler_t disp);
int sigsuspend(const sigset_t *set);
int sigtimedwait(const sigset_t *__restrict set, siginfo_t *__restrict info, const struct timespec *__restrict tm);
int sigwait(const sigset_t *__restrict set, int *__restrict sig);
int sigwaitinfo(const sigset_t *__restrict set, siginfo_t *__restrict info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define SIG_DFL ((sighandler_t) 0)
#define SIG_ERR ((sighandler_t) -1)
#define SIG_IGN ((sighandler_t) 1)

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#endif /* _SIGNAL_H */