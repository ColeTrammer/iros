#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1

#define ENUMERATE_SYSCALLS                                                   \
    __ENUMERATE_SYSCALL(EXIT, exit, 1)                                       \
    __ENUMERATE_SYSCALL(SBRK, sbrk, 1)                                       \
    __ENUMERATE_SYSCALL(FORK, fork, 0)                                       \
    __ENUMERATE_SYSCALL(OPENAT, openat, 4)                                   \
    __ENUMERATE_SYSCALL(READ, read, 3)                                       \
    __ENUMERATE_SYSCALL(WRITE, write, 3)                                     \
    __ENUMERATE_SYSCALL(CLOSE, close, 1)                                     \
    __ENUMERATE_SYSCALL(EXECVE, execve, 3)                                   \
    __ENUMERATE_SYSCALL(WAITPID, waitpid, 3)                                 \
    __ENUMERATE_SYSCALL(GETPID, getpid, 0)                                   \
    __ENUMERATE_SYSCALL(GETCWD, getcwd, 2)                                   \
    __ENUMERATE_SYSCALL(CHDIR, chdir, 1)                                     \
    __ENUMERATE_SYSCALL(FSTATAT, fstatat, 4)                                 \
    __ENUMERATE_SYSCALL(LSEEK, lseek, 3)                                     \
    __ENUMERATE_SYSCALL(IOCTL, ioctl, 3)                                     \
    __ENUMERATE_SYSCALL(FTRUNCATE, ftruncate, 2)                             \
    __ENUMERATE_SYSCALL(GETTIMEOFDAY, gettimeofday, 2)                       \
    __ENUMERATE_SYSCALL(MKDIR, mkdir, 2)                                     \
    __ENUMERATE_SYSCALL(DUP2, dup2, 2)                                       \
    __ENUMERATE_SYSCALL(PIPE, pipe, 1)                                       \
    __ENUMERATE_SYSCALL(UNLINK, unlink, 1)                                   \
    __ENUMERATE_SYSCALL(RMDIR, rmdir, 1)                                     \
    __ENUMERATE_SYSCALL(FCHMODAT, fchmodat, 4)                               \
    __ENUMERATE_SYSCALL(KILL, kill, 2)                                       \
    __ENUMERATE_SYSCALL(SETPGID, setpgid, 2)                                 \
    __ENUMERATE_SYSCALL(SIGACTION, sigaction, 3)                             \
    __ENUMERATE_SYSCALL(SIGRETURN, sigreturn, 0)                             \
    __ENUMERATE_SYSCALL(SIGPROCMASK, sigprocmask, 3)                         \
    __ENUMERATE_SYSCALL(DUP, dup, 1)                                         \
    __ENUMERATE_SYSCALL(GETPGID, getpgid, 1)                                 \
    __ENUMERATE_SYSCALL(FACCESSAT, faccessat, 4)                             \
    __ENUMERATE_SYSCALL(ACCEPT4, accept4, 4)                                 \
    __ENUMERATE_SYSCALL(BIND, bind, 3)                                       \
    __ENUMERATE_SYSCALL(CONNECT, connect, 3)                                 \
    __ENUMERATE_SYSCALL(LISTEN, listen, 2)                                   \
    __ENUMERATE_SYSCALL(SOCKET, socket, 3)                                   \
    __ENUMERATE_SYSCALL(SHUTDOWN, shutdown, 2)                               \
    __ENUMERATE_SYSCALL(GETSOCKOPT, getsockopt, 5)                           \
    __ENUMERATE_SYSCALL(SETSOCKOPT, setsockopt, 5)                           \
    __ENUMERATE_SYSCALL(GETPEERNAME, getpeername, 3)                         \
    __ENUMERATE_SYSCALL(GETSOCKNAME, getsockname, 3)                         \
    __ENUMERATE_SYSCALL(SENDTO, sendto, 6)                                   \
    __ENUMERATE_SYSCALL(RECVFROM, recvfrom, 6)                               \
    __ENUMERATE_SYSCALL(MMAP, mmap, 6)                                       \
    __ENUMERATE_SYSCALL(MUNMAP, munmap, 2)                                   \
    __ENUMERATE_SYSCALL(RENAME, rename, 2)                                   \
    __ENUMERATE_SYSCALL(FCNTL, fcntl, 3)                                     \
    __ENUMERATE_SYSCALL(ALARM, alarm, 1)                                     \
    __ENUMERATE_SYSCALL(GETPPID, getppid, 0)                                 \
    __ENUMERATE_SYSCALL(SIGSUSPEND, sigsuspend, 1)                           \
    __ENUMERATE_SYSCALL(TIMES, times, 1)                                     \
    __ENUMERATE_SYSCALL(CREATE_TASK, create_task, 1)                         \
    __ENUMERATE_SYSCALL(EXIT_TASK, exit_task, 0)                             \
    __ENUMERATE_SYSCALL(OS_MUTEX, os_mutex, 6)                               \
    __ENUMERATE_SYSCALL(TGKILL, tgkill, 3)                                   \
    __ENUMERATE_SYSCALL(SET_THREAD_SELF_POINTER, set_thread_self_pointer, 2) \
    __ENUMERATE_SYSCALL(MPROTECT, mprotect, 3)                               \
    __ENUMERATE_SYSCALL(SIGPENDING, sigpending, 1)                           \
    __ENUMERATE_SYSCALL(SIGALTSTACK, sigaltstack, 2)                         \
    __ENUMERATE_SYSCALL(PSELECT, pselect, 6)                                 \
    __ENUMERATE_SYSCALL(SETUID, setuid, 1)                                   \
    __ENUMERATE_SYSCALL(SETEUID, seteuid, 1)                                 \
    __ENUMERATE_SYSCALL(SETGID, setgid, 1)                                   \
    __ENUMERATE_SYSCALL(SETEGID, setegid, 1)                                 \
    __ENUMERATE_SYSCALL(GETUID, getuid, 0)                                   \
    __ENUMERATE_SYSCALL(GETEUID, geteuid, 0)                                 \
    __ENUMERATE_SYSCALL(GETGID, getgid, 0)                                   \
    __ENUMERATE_SYSCALL(GETEGID, getegid, 0)                                 \
    __ENUMERATE_SYSCALL(UMASK, umask, 1)                                     \
    __ENUMERATE_SYSCALL(UNAME, uname, 1)                                     \
    __ENUMERATE_SYSCALL(GETSID, getsid, 1)                                   \
    __ENUMERATE_SYSCALL(SETSID, setsid, 0)                                   \
    __ENUMERATE_SYSCALL(READLINK, readlink, 3)                               \
    __ENUMERATE_SYSCALL(SYMLINK, symlink, 2)                                 \
    __ENUMERATE_SYSCALL(LINK, link, 2)                                       \
    __ENUMERATE_SYSCALL(FCHOWNAT, fchownat, 5)                               \
    __ENUMERATE_SYSCALL(UTIMENSAT, utimensat, 4)                             \
    __ENUMERATE_SYSCALL(PREAD, pread, 4)                                     \
    __ENUMERATE_SYSCALL(PWRITE, pwrite, 4)                                   \
    __ENUMERATE_SYSCALL(READV, readv, 3)                                     \
    __ENUMERATE_SYSCALL(WRITEV, writev, 3)                                   \
    __ENUMERATE_SYSCALL(REALPATH, realpath, 3)                               \
    __ENUMERATE_SYSCALL(CLOCK_NANOSLEEP, clock_nanosleep, 4)                 \
    __ENUMERATE_SYSCALL(CLOCK_GETRES, clock_getres, 2)                       \
    __ENUMERATE_SYSCALL(CLOCK_GETTIME, clock_gettime, 2)                     \
    __ENUMERATE_SYSCALL(CLOCK_SETTIME, clock_settime, 2)                     \
    __ENUMERATE_SYSCALL(GETCPUCLOCKID, getcpuclockid, 3)                     \
    __ENUMERATE_SYSCALL(SIGQUEUE, sigqueue, 3)                               \
    __ENUMERATE_SYSCALL(TIMER_CREATE, timer_create, 3)                       \
    __ENUMERATE_SYSCALL(TIMER_DELETE, timer_delete, 1)                       \
    __ENUMERATE_SYSCALL(TIMER_GETOVERRUN, timer_getoverrun, 1)               \
    __ENUMERATE_SYSCALL(TIMER_GETTIME, timer_gettime, 2)                     \
    __ENUMERATE_SYSCALL(TIMER_SETTIME, timer_settime, 4)                     \
    __ENUMERATE_SYSCALL(FSTATVFS, fstatvfs, 2)                               \
    __ENUMERATE_SYSCALL(STATVFS, statvfs, 2)                                 \
    __ENUMERATE_SYSCALL(FCHDIR, fchdir, 1)                                   \
    __ENUMERATE_SYSCALL(TRUNCATE, truncate, 2)                               \
    __ENUMERATE_SYSCALL(GETGROUPS, getgroups, 2)                             \
    __ENUMERATE_SYSCALL(SETGROUPS, setgroups, 2)                             \
    __ENUMERATE_SYSCALL(MKNOD, mknod, 3)                                     \
    __ENUMERATE_SYSCALL(PPOLL, ppoll, 4)                                     \
    __ENUMERATE_SYSCALL(ENABLE_PROFILING, enable_profiling, 1)               \
    __ENUMERATE_SYSCALL(READ_PROFILE, read_profile, 3)                       \
    __ENUMERATE_SYSCALL(DISABLE_PROFILING, disable_profiling, 1)             \
    __ENUMERATE_SYSCALL(GETRLIMIT, getrlimit, 2)                             \
    __ENUMERATE_SYSCALL(SETRLIMIT, setrlimit, 2)                             \
    __ENUMERATE_SYSCALL(SOCKETPAIR, socketpair, 4)

#ifdef __ASSEMBLER__
#define SYS_SIGRETURN 27
#else

#ifdef __cplusplus
extern "C" {
#define _Static_assert static_assert
#endif /* __cplusplus */

enum sc_number {
#define __ENUMERATE_SYSCALL(x, y, v) SYS_##x,
    __SYS_START,
    ENUMERATE_SYSCALLS __SYS_NUM
};

#ifdef __is_libc
#if ARCH == X86_64
#define __SC_TYPE(name)   __typeof__((name) - (name))
#define __SC_CAST(name)   ((__SC_TYPE(name))(name))
#define __SC_CLOBBER_LIST "cc", "cx", "r11"

#define __syscall0(sc)                                                                 \
    ({                                                                                 \
        long ret;                                                                      \
        asm volatile("syscall\n" : "=a"(ret) : "0"(sc) : "memory", __SC_CLOBBER_LIST); \
        ret;                                                                           \
    })

#define __syscall1(sc, a1)                                                                        \
    ({                                                                                            \
        long ret;                                                                                 \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                                      \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                                           \
        asm volatile("syscall\n" : "=a"(ret) : "0"(sc), "r"(__a1) : "memory", __SC_CLOBBER_LIST); \
        ret;                                                                                      \
    })

#define __syscall2(sc, a1, a2)                                                                               \
    ({                                                                                                       \
        long ret;                                                                                            \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                                                 \
        __SC_TYPE(a2) __a2_ = __SC_CAST(a2);                                                                 \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                                                      \
        register __SC_TYPE(a2) __a2 asm("rsi") = __a2_;                                                      \
        asm volatile("syscall\n" : "=a"(ret) : "0"(sc), "r"(__a1), "r"(__a2) : "memory", __SC_CLOBBER_LIST); \
        ret;                                                                                                 \
    })

#define __syscall3(sc, a1, a2, a3)                                                                                      \
    ({                                                                                                                  \
        long ret;                                                                                                       \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                                                            \
        __SC_TYPE(a2) __a2_ = __SC_CAST(a2);                                                                            \
        __SC_TYPE(a3) __a3_ = __SC_CAST(a3);                                                                            \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                                                                 \
        register __SC_TYPE(a2) __a2 asm("rsi") = __a2_;                                                                 \
        register __SC_TYPE(a3) __a3 asm("rdx") = __a3_;                                                                 \
        asm volatile("syscall\n" : "=a"(ret) : "0"(sc), "r"(__a1), "r"(__a2), "r"(__a3) : "memory", __SC_CLOBBER_LIST); \
        ret;                                                                                                            \
    })

#define __syscall4(sc, a1, a2, a3, a4)                                                                                             \
    ({                                                                                                                             \
        long ret;                                                                                                                  \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                                                                       \
        __SC_TYPE(a2) __a2_ = __SC_CAST(a2);                                                                                       \
        __SC_TYPE(a3) __a3_ = __SC_CAST(a3);                                                                                       \
        __SC_TYPE(a4) __a4_ = __SC_CAST(a4);                                                                                       \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                                                                            \
        register __SC_TYPE(a2) __a2 asm("rsi") = __a2_;                                                                            \
        register __SC_TYPE(a3) __a3 asm("rdx") = __a3_;                                                                            \
        register __SC_TYPE(a4) __a4 asm("r8") = __a4_;                                                                             \
        asm volatile("syscall\n" : "=a"(ret) : "0"(sc), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4) : "memory", __SC_CLOBBER_LIST); \
        ret;                                                                                                                       \
    })

#define __syscall5(sc, a1, a2, a3, a4, a5)                                            \
    ({                                                                                \
        long ret;                                                                     \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                          \
        __SC_TYPE(a2) __a2_ = __SC_CAST(a2);                                          \
        __SC_TYPE(a3) __a3_ = __SC_CAST(a3);                                          \
        __SC_TYPE(a4) __a4_ = __SC_CAST(a4);                                          \
        __SC_TYPE(a5) __a5_ = __SC_CAST(a5);                                          \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                               \
        register __SC_TYPE(a2) __a2 asm("rsi") = __a2_;                               \
        register __SC_TYPE(a3) __a3 asm("rdx") = __a3_;                               \
        register __SC_TYPE(a4) __a4 asm("r8") = __a4_;                                \
        register __SC_TYPE(a5) __a5 asm("r9") = __a5_;                                \
        asm volatile("syscall\n"                                                      \
                     : "=a"(ret)                                                      \
                     : "0"(sc), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4), "r"(__a5) \
                     : "memory", __SC_CLOBBER_LIST);                                  \
        ret;                                                                          \
    })

#define __syscall6(sc, a1, a2, a3, a4, a5, a6)                                                   \
    ({                                                                                           \
        long ret;                                                                                \
        __SC_TYPE(a1) __a1_ = __SC_CAST(a1);                                                     \
        __SC_TYPE(a2) __a2_ = __SC_CAST(a2);                                                     \
        __SC_TYPE(a3) __a3_ = __SC_CAST(a3);                                                     \
        __SC_TYPE(a4) __a4_ = __SC_CAST(a4);                                                     \
        __SC_TYPE(a5) __a5_ = __SC_CAST(a5);                                                     \
        __SC_TYPE(a6) __a6_ = __SC_CAST(a6);                                                     \
        register __SC_TYPE(a1) __a1 asm("rdi") = __a1_;                                          \
        register __SC_TYPE(a2) __a2 asm("rsi") = __a2_;                                          \
        register __SC_TYPE(a3) __a3 asm("rdx") = __a3_;                                          \
        register __SC_TYPE(a4) __a4 asm("r8") = __a4_;                                           \
        register __SC_TYPE(a5) __a5 asm("r9") = __a5_;                                           \
        register __SC_TYPE(a6) __a6 asm("r10") = __a6_;                                          \
        asm volatile("syscall\n"                                                                 \
                     : "=a"(ret)                                                                 \
                     : "0"(sc), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4), "r"(__a5), "r"(__a6) \
                     : "memory", __SC_CLOBBER_LIST);                                             \
        ret;                                                                                     \
    })
#else
#erro "Unsupported architecture"
#endif /* ARCH */
#else
long __do_syscall(int sc, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6);

#define __syscall0(n)          __do_syscall(n, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL)
#define __syscall1(n, a)       __do_syscall(n, (unsigned long) (a), 0UL, 0UL, 0UL, 0UL, 0UL)
#define __syscall2(n, a, b)    __do_syscall(n, (unsigned long) (a), (unsigned long) (b), 0UL, 0UL, 0UL, 0UL)
#define __syscall3(n, a, b, c) __do_syscall(n, (unsigned long) (a), (unsigned long) (b), (unsigned long) (c), 0UL, 0UL, 0UL)
#define __syscall4(n, a, b, c, d) \
    __do_syscall(n, (unsigned long) (a), (unsigned long) (b), (unsigned long) (c), (unsigned long) (d), 0UL, 0UL)
#define __syscall5(n, a, b, c, d, e) \
    __do_syscall(n, (unsigned long) (a), (unsigned long) (b), (unsigned long) (c), (unsigned long) (d), (unsigned long) (e), 0UL)
#define __syscall6(n, a, b, c, d, e, f)                                                                                      \
    __do_syscall(n, (unsigned long) (a), (unsigned long) (b), (unsigned long) (c), (unsigned long) (d), (unsigned long) (e), \
                 (unsigned long) (f))
#endif /* __is_libc */

#define __COUNT_ARGS(...)                               __COUNT_ARGS_(, ##__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)
#define __COUNT_ARGS_(z, a, b, c, d, e, f, g, cnt, ...) cnt

#define __SYSCALL_WITH_ARG_COUNT(...)  __SYSCALL_WITH_ARG_COUNT_(__VA_ARGS__)
#define __SYSCALL_WITH_ARG_COUNT_(...) __syscall##__VA_ARGS__

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x, y, v) __SYS_##x##_ARG_COUNT = v,
enum __sc_arg_count { ENUMERATE_SYSCALLS __INVALID };

#ifdef __is_libc
#define syscall(n, ...)                                                                                              \
    ({                                                                                                               \
        _Static_assert(__COUNT_ARGS(__VA_ARGS__) == __##n##_ARG_COUNT, "Incorrect number of arguments for " #n "."); \
        __SYSCALL_WITH_ARG_COUNT(__COUNT_ARGS(__VA_ARGS__))(n, ##__VA_ARGS__);                                       \
    })
#else
#define syscall(n, ...) __SYSCALL_WITH_ARG_COUNT(__COUNT_ARGS(__VA_ARGS__))(n, ##__VA_ARGS__)
#endif /* __is_libc */

char *syscall_to_string(enum sc_number sc);

__attribute__((__noreturn__)) void __sigreturn(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__ASSEMBLER */

#endif /* _SYS_SYSCALL_H */
