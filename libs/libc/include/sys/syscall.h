#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1

#define ENUMERATE_SYSCALLS                          \
    __ENUMERATE_SYSCALL(exit, 1)                    \
    __ENUMERATE_SYSCALL(sbrk, 1)                    \
    __ENUMERATE_SYSCALL(fork, 0)                    \
    __ENUMERATE_SYSCALL(openat, 4)                  \
    __ENUMERATE_SYSCALL(read, 3)                    \
    __ENUMERATE_SYSCALL(write, 3)                   \
    __ENUMERATE_SYSCALL(close, 1)                   \
    __ENUMERATE_SYSCALL(execve, 3)                  \
    __ENUMERATE_SYSCALL(waitpid, 3)                 \
    __ENUMERATE_SYSCALL(getpid, 0)                  \
    __ENUMERATE_SYSCALL(getcwd, 2)                  \
    __ENUMERATE_SYSCALL(chdir, 1)                   \
    __ENUMERATE_SYSCALL(fstatat, 4)                 \
    __ENUMERATE_SYSCALL(lseek, 3)                   \
    __ENUMERATE_SYSCALL(ioctl, 3)                   \
    __ENUMERATE_SYSCALL(ftruncate, 2)               \
    __ENUMERATE_SYSCALL(gettimeofday, 2)            \
    __ENUMERATE_SYSCALL(mkdir, 2)                   \
    __ENUMERATE_SYSCALL(dup2, 2)                    \
    __ENUMERATE_SYSCALL(pipe, 1)                    \
    __ENUMERATE_SYSCALL(unlink, 1)                  \
    __ENUMERATE_SYSCALL(rmdir, 1)                   \
    __ENUMERATE_SYSCALL(fchmodat, 4)                \
    __ENUMERATE_SYSCALL(kill, 2)                    \
    __ENUMERATE_SYSCALL(setpgid, 2)                 \
    __ENUMERATE_SYSCALL(sigaction, 3)               \
    __ENUMERATE_SYSCALL(sigreturn, 0)               \
    __ENUMERATE_SYSCALL(sigprocmask, 3)             \
    __ENUMERATE_SYSCALL(dup, 1)                     \
    __ENUMERATE_SYSCALL(getpgid, 1)                 \
    __ENUMERATE_SYSCALL(faccessat, 4)               \
    __ENUMERATE_SYSCALL(accept4, 4)                 \
    __ENUMERATE_SYSCALL(bind, 3)                    \
    __ENUMERATE_SYSCALL(connect, 3)                 \
    __ENUMERATE_SYSCALL(listen, 2)                  \
    __ENUMERATE_SYSCALL(socket, 3)                  \
    __ENUMERATE_SYSCALL(shutdown, 2)                \
    __ENUMERATE_SYSCALL(getsockopt, 5)              \
    __ENUMERATE_SYSCALL(setsockopt, 5)              \
    __ENUMERATE_SYSCALL(getpeername, 3)             \
    __ENUMERATE_SYSCALL(getsockname, 3)             \
    __ENUMERATE_SYSCALL(sendto, 6)                  \
    __ENUMERATE_SYSCALL(recvfrom, 6)                \
    __ENUMERATE_SYSCALL(mmap, 6)                    \
    __ENUMERATE_SYSCALL(munmap, 2)                  \
    __ENUMERATE_SYSCALL(rename, 2)                  \
    __ENUMERATE_SYSCALL(fcntl, 3)                   \
    __ENUMERATE_SYSCALL(getppid, 0)                 \
    __ENUMERATE_SYSCALL(sigsuspend, 1)              \
    __ENUMERATE_SYSCALL(times, 1)                   \
    __ENUMERATE_SYSCALL(create_task, 1)             \
    __ENUMERATE_SYSCALL(exit_task, 0)               \
    __ENUMERATE_SYSCALL(os_mutex, 6)                \
    __ENUMERATE_SYSCALL(tgkill, 3)                  \
    __ENUMERATE_SYSCALL(set_thread_self_pointer, 2) \
    __ENUMERATE_SYSCALL(mprotect, 3)                \
    __ENUMERATE_SYSCALL(sigpending, 1)              \
    __ENUMERATE_SYSCALL(sigaltstack, 2)             \
    __ENUMERATE_SYSCALL(pselect, 6)                 \
    __ENUMERATE_SYSCALL(setuid, 1)                  \
    __ENUMERATE_SYSCALL(seteuid, 1)                 \
    __ENUMERATE_SYSCALL(setgid, 1)                  \
    __ENUMERATE_SYSCALL(setegid, 1)                 \
    __ENUMERATE_SYSCALL(getuid, 0)                  \
    __ENUMERATE_SYSCALL(geteuid, 0)                 \
    __ENUMERATE_SYSCALL(getgid, 0)                  \
    __ENUMERATE_SYSCALL(getegid, 0)                 \
    __ENUMERATE_SYSCALL(umask, 1)                   \
    __ENUMERATE_SYSCALL(uname, 1)                   \
    __ENUMERATE_SYSCALL(getsid, 1)                  \
    __ENUMERATE_SYSCALL(setsid, 0)                  \
    __ENUMERATE_SYSCALL(readlink, 3)                \
    __ENUMERATE_SYSCALL(symlink, 2)                 \
    __ENUMERATE_SYSCALL(link, 2)                    \
    __ENUMERATE_SYSCALL(fchownat, 5)                \
    __ENUMERATE_SYSCALL(utimensat, 4)               \
    __ENUMERATE_SYSCALL(pread, 4)                   \
    __ENUMERATE_SYSCALL(pwrite, 4)                  \
    __ENUMERATE_SYSCALL(readv, 3)                   \
    __ENUMERATE_SYSCALL(writev, 3)                  \
    __ENUMERATE_SYSCALL(realpath, 3)                \
    __ENUMERATE_SYSCALL(clock_nanosleep, 4)         \
    __ENUMERATE_SYSCALL(clock_getres, 2)            \
    __ENUMERATE_SYSCALL(clock_gettime, 2)           \
    __ENUMERATE_SYSCALL(clock_settime, 2)           \
    __ENUMERATE_SYSCALL(getcpuclockid, 3)           \
    __ENUMERATE_SYSCALL(sigqueue, 3)                \
    __ENUMERATE_SYSCALL(timer_create, 3)            \
    __ENUMERATE_SYSCALL(timer_delete, 1)            \
    __ENUMERATE_SYSCALL(timer_getoverrun, 1)        \
    __ENUMERATE_SYSCALL(timer_gettime, 2)           \
    __ENUMERATE_SYSCALL(timer_settime, 4)           \
    __ENUMERATE_SYSCALL(fstatvfs, 2)                \
    __ENUMERATE_SYSCALL(statvfs, 2)                 \
    __ENUMERATE_SYSCALL(fchdir, 1)                  \
    __ENUMERATE_SYSCALL(truncate, 2)                \
    __ENUMERATE_SYSCALL(getgroups, 2)               \
    __ENUMERATE_SYSCALL(setgroups, 2)               \
    __ENUMERATE_SYSCALL(mknod, 3)                   \
    __ENUMERATE_SYSCALL(ppoll, 4)                   \
    __ENUMERATE_SYSCALL(enable_profiling, 1)        \
    __ENUMERATE_SYSCALL(read_profile, 3)            \
    __ENUMERATE_SYSCALL(disable_profiling, 1)       \
    __ENUMERATE_SYSCALL(getrlimit, 2)               \
    __ENUMERATE_SYSCALL(setrlimit, 2)               \
    __ENUMERATE_SYSCALL(socketpair, 4)              \
    __ENUMERATE_SYSCALL(sched_yield, 0)             \
    __ENUMERATE_SYSCALL(getrusage, 2)               \
    __ENUMERATE_SYSCALL(nice, 1)                    \
    __ENUMERATE_SYSCALL(getpriority, 2)             \
    __ENUMERATE_SYSCALL(setprioity, 3)              \
    __ENUMERATE_SYSCALL(getitimer, 2)               \
    __ENUMERATE_SYSCALL(setitimer, 3)               \
    __ENUMERATE_SYSCALL(mount, 5)                   \
    __ENUMERATE_SYSCALL(umount, 1)

#ifdef __ASSEMBLER__
#define SYS_SIGRETURN 27
#else

#ifdef __cplusplus
extern "C" {
#define _Static_assert static_assert
#endif /* __cplusplus */

enum sc_number {
#define __ENUMERATE_SYSCALL(x, v) SYS_##x,
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
#define __ENUMERATE_SYSCALL(x, v) __SYS_##x##_ARG_COUNT = v,
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
