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
#define SIGWINCH  29
// 30 - 31 are reserved for pthread implemenation
#ifdef __is_kernel
#define SIGRTMIN 30
#else
#define SIGRTMIN 32
#endif /* __is_kernel */
#define SIGRTMAX 63
#define _NSIG    64

#define SI_USER       0
#define SI_QUEUE      1
#define SI_TIMER      2
#define SI_ASYNCIO    3
#define SI_MESGQ      4
#define ILL_ILLOPC    5
#define ILL_ILLOPN    6
#define ILL_ILLADR    7
#define ILL_ILLTRP    8
#define ILL_PRVOPC    9
#define ILL_PRVREG    10
#define ILL_COPROC    11
#define ILL_BADSTK    12
#define FPE_INTDIV    13
#define FPE_INTOVF    14
#define FPE_FLTDIV    15
#define FPE_FLTOVF    16
#define FPE_FLTUND    17
#define FPE_FLTRES    18
#define FPE_FLTINV    19
#define FPE_FLTSUB    20
#define SEGV_MAPERR   21
#define SEGV_ACCERR   22
#define BUS_ADRALN    23
#define BUS_ADRERR    24
#define TRAP_BRKPT    25
#define TRAP_TRACE    26
#define CLD_EXITED    27
#define CLD_KILLED    28
#define CLD_DUMPED    29
#define CLD_TRAPPED   30
#define CLD_STOPPED   31
#define CLD_CONTINUED 32
#define POLL_IN       33
#define POLL_OUT      34
#define POLL_MSG      35
#define POLL_ERR      36
#define POLL_PRI      37
#define POLL_HUP      38

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const char *const sys_siglist[];

typedef int sig_atomic_t;
typedef __UINT64_TYPE__ sigset_t;

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
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } __sigaction_handler;
#define sa_handler   __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

#ifdef __x86_64__

typedef struct {
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rbp;
    unsigned long rdi;
    unsigned long rsi;
    unsigned long rdx;
    unsigned long rcx;
    unsigned long rbx;
    unsigned long rax;
} __attribute__((packed)) __cpu_state_t;

typedef struct {
    unsigned long rip;
    unsigned long cs;
    unsigned long rflags;
    unsigned long rsp;
    unsigned long ss;
} __attribute__((packed)) __stack_state_t;

typedef struct {
    __cpu_state_t __cpu_state;
    __stack_state_t __stack_state;
} __attribute__((packed)) mcontext_t;

#endif /* __x86_64__ */

typedef struct __ucontext {
    struct __ucontext *uc_link;
    sigset_t uc_sigmask;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
} ucontext_t;

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

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SIGEV_NONE   0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2
#ifdef __is_kernel
#define SIGEV_KERNEL 3
#endif /* __is_kernel */

#define SA_NOCLDSTOP 1
#define SA_ONSTACK   2
#define SA_RESETHAND 4
#define SA_RESTART   8
#define SA_SIGINFO   16
#define SA_NOCLDWAIT 32
#define SA_NODEFER   64

#define SS_ONSTACK 1
#define SS_DISABLE 2

#define MINSIGSTKSZ 4096
#define SIGSTKSZ    (4096 * 4)

#ifdef __is_kernel
#define __SS_ENABLED 4
#endif /* __is_kernel */

#endif /* _SIGNAL_H */