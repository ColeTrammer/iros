#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/os_2.h>
#include <sys/syscall.h>
#include <unistd.h>

int kill(pid_t pid, int sig) {
    int ret = (int) syscall(SC_KILL, pid, sig);
    __SYSCALL_TO_ERRNO(ret);
}

int killpg(int pgid, int sig) {
    return kill(-pgid, sig);
}

char *strsignal(int sig) {
    if (sig < 0 || sig > _NSIG) {
        sig = 0;
    }

    return (char *) sys_siglist[sig];
}

void psignal(int sig, const char *s) {
    if (s && s[0] != '\0') {
        fprintf(stderr, "%s: ", s);
    }

    fputs(strsignal(sig), stderr);
}

int raise(int signum) {
    // Make sure to signal the current thread, not just a random one in the process
    return tgkill(0, 0, signum);
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *old_act) {
    ((struct sigaction *) act)->sa_restorer = &__sigreturn;
    int ret = (int) syscall(SC_SIGACTION, signum, act, old_act);
    __SYSCALL_TO_ERRNO(ret);
}

sighandler_t signal(int signum, sighandler_t handler) {
    struct sigaction act;
    struct sigaction old;

    act.sa_handler = handler;
    act.sa_flags = SA_RESTART; // glibc does this
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old)) {
        return SIG_ERR;
    }

    return old.sa_handler;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *old) {
    int ret = (int) syscall(SC_SIGPROCMASK, how, set, old);
    __SYSCALL_TO_ERRNO(ret);
}

int sigpending(sigset_t *set) {
    int ret = (int) syscall(SC_SIGPENDING, set);
    __SYSCALL_TO_ERRNO(ret);
}

int sigsuspend(const sigset_t *set) {
    int ret = (int) syscall(SC_SIGSUSPEND, set);
    __SYSCALL_TO_ERRNO(ret);
}

int sigaltstack(const stack_t *ss, stack_t *old_ss) {
    int ret = (int) syscall(SC_SIGALTSTACK, ss, old_ss);
    __SYSCALL_TO_ERRNO(ret);
}

int sigemptyset(sigset_t *set) {
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set) {
    *set = ~0;
    return 0;
}

int sigaddset(sigset_t *set, int signum) {
    if (signum < 1 || signum > _NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set |= (1U << (signum - 1));
    return 0;
}

int sigdelset(sigset_t *set, int signum) {
    if (signum < 1 || signum > _NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1U << (signum - 1));
    return 0;
}

int sigismember(const sigset_t *set, int signum) {
    if (signum < 1 || signum >= _NSIG) {
        errno = EINVAL;
        return -1;
    }
    return *set & (1U << (signum - 1)) ? 1 : 0;
}

int siginterrupt(int sig, int flag) {
    struct sigaction act;

    if (sig <= 0 || sig >= _NSIG) {
        errno = -EINVAL;
        return -1;
    }

    sigaction(sig, NULL, &act);
    if (flag == 0) {
        act.sa_flags |= SA_RESTART;
    } else {
        act.sa_flags &= ~SA_RESTART;
    }

    return sigaction(sig, &act, NULL);
}

// Called from sigsetjmp assembly function after registers are set
int __sigsetjmp(sigjmp_buf env, int val) {
    if (val) {
        env->is_mask_saved = 1;
        sigprocmask(0, NULL, &env->mask);
    } else {
        env->is_mask_saved = 0;
    }
    return 0;
}

__attribute__((noreturn)) void siglongjmp(sigjmp_buf env, int val) {
    if (env->is_mask_saved) {
        sigprocmask(SIG_SETMASK, &env->mask, NULL);
    }

    longjmp(env, val);
}

const char *const sys_siglist[_NSIG] = {
    "Invalid signal number",
    "TTY hang up",
    "Interrupted",
    "Quit",
    "Bus",
    "Trap",
    "Aborted",
    "Continued",
    "Floating point exeception",
    "Killed",
    "Read from tty",
    "Write to tty",
    "Illegal instruction",
    "Pipe error",
    "Alarm",
    "Terminated",
    "Segmentation fault",
    "Stopped",
    "Stopped by tty",
    "User 1",
    "User 2",
    "Poll",
    "Profile",
    "Invalid system call",
    "Urge",
    "Virtual alarm",
    "CPU exceeded time limit",
    "File size limit exceeded",
    "Child state change",
    "Invalid signal number",
    "Invalid signal number",
    "Invalid signal number",
};