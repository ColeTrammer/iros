#ifndef _SIGNAL_H
#define _SIGNAL_H 1

#include <bits/pthread_attr_t.h>
#include <bits/pthread_t.h>
#include <bits/size_t.h>
#include <bits/uid_t.h>
#include <time.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGBUS    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGCONT   7
#define SIGFPE    8
#define SIGKILL   9
#define SIGTTIN   10
#define SIGTTOU   11
#define SIGILL    12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSEGV   16
#define SIGSTOP   17
#define SIGTSTP   18
#define SIGUSR1   19
#define SIGUSR2   20
#define SIGPOLL   21
#define SIGPROF   22
#define SIGSYS    23
#define SIGURG    24
#define SIGVTALRM 25
#define SIGXCPU   26
#define SIGXFSZ   27
#define SIGCHLD   28
#define SIGCLD    SIGCHLD
#define _NSIG     32

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const char *const sys_siglist[];

typedef int sig_atomic_t;
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
    void (*sa_restorer)(void);
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

#ifdef __libc_internal
__attribute__((noreturn))
void sigreturn(void);
#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define SIG_DFL ((sighandler_t) 0)
#define SIG_ERR ((sighandler_t) -1)
#define SIG_IGN ((sighandler_t) 1)

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SA_NOCLDSTOP 1
#define SA_ONSTACK 2
#define SA_RESETHAND 4
#define SA_RESTART 8
#define SA_SIGINFO 16
#define SA_NOCLDWAIT 32
#define SA_NODEFER 64

#endif /* _SIGNAL_H */