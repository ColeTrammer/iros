#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1

#define ENUMERATE_SYSCALLS                           \
    __ENUMERATE_SYSCALL(EXIT, 1)                     \
    __ENUMERATE_SYSCALL(SBRK, 1)                     \
    __ENUMERATE_SYSCALL(FORK, 0)                     \
    __ENUMERATE_SYSCALL(OPEN, 3)                     \
    __ENUMERATE_SYSCALL(READ, 3)                     \
    __ENUMERATE_SYSCALL(WRITE, 3)                    \
    __ENUMERATE_SYSCALL(CLOSE, 1)                    \
    __ENUMERATE_SYSCALL(EXECVE, 3)                   \
    __ENUMERATE_SYSCALL(WAITPID, 3)                  \
    __ENUMERATE_SYSCALL(GETPID, 0)                   \
    __ENUMERATE_SYSCALL(GETCWD, 2)                   \
    __ENUMERATE_SYSCALL(CHDIR, 1)                    \
    __ENUMERATE_SYSCALL(STAT, 2)                     \
    __ENUMERATE_SYSCALL(LSEEK, 3)                    \
    __ENUMERATE_SYSCALL(IOCTL, 3)                    \
    __ENUMERATE_SYSCALL(FTRUNCATE, 2)                \
    __ENUMERATE_SYSCALL(GETTIMEOFDAY, 3)             \
    __ENUMERATE_SYSCALL(MKDIR, 2)                    \
    __ENUMERATE_SYSCALL(DUP2, 2)                     \
    __ENUMERATE_SYSCALL(PIPE, 1)                     \
    __ENUMERATE_SYSCALL(UNLINK, 1)                   \
    __ENUMERATE_SYSCALL(RMDIR, 1)                    \
    __ENUMERATE_SYSCALL(CHMOD, 1)                    \
    __ENUMERATE_SYSCALL(KILL, 2)                     \
    __ENUMERATE_SYSCALL(SETPGID, 2)                  \
    __ENUMERATE_SYSCALL(SIGACTION, 3)                \
    __ENUMERATE_SYSCALL(SIGRETURN, 0)                \
    __ENUMERATE_SYSCALL(SIGPROCMASK, 3)              \
    __ENUMERATE_SYSCALL(DUP, 1)                      \
    __ENUMERATE_SYSCALL(GETPGID, 1)                  \
    __ENUMERATE_SYSCALL(SLEEP, 1)                    \
    __ENUMERATE_SYSCALL(ACCESS, 2)                   \
    __ENUMERATE_SYSCALL(ACCEPT4, 4)                  \
    __ENUMERATE_SYSCALL(BIND, 3)                     \
    __ENUMERATE_SYSCALL(CONNECT, 3)                  \
    __ENUMERATE_SYSCALL(LISTEN, 2)                   \
    __ENUMERATE_SYSCALL(SOCKET, 3)                   \
    __ENUMERATE_SYSCALL(SHUTDOWN, 2)                 \
    __ENUMERATE_SYSCALL(GETSOCKOPT, 5)               \
    __ENUMERATE_SYSCALL(SETSOCKOPT, 5)               \
    __ENUMERATE_SYSCALL(GETPEERNAME, 3)              \
    __ENUMERATE_SYSCALL(GETSOCKNAME, 3)              \
    __ENUMERATE_SYSCALL(SENDTO, 6)                   \
    __ENUMERATE_SYSCALL(RECVFROM, 6)                 \
    __ENUMERATE_SYSCALL(MMAP, 6)                     \
    __ENUMERATE_SYSCALL(MUNMAP, 2)                   \
    __ENUMERATE_SYSCALL(RENAME, 2)                   \
    __ENUMERATE_SYSCALL(FCNTL, 3)                    \
    __ENUMERATE_SYSCALL(FSTAT, 2)                    \
    __ENUMERATE_SYSCALL(ALARM, 1)                    \
    __ENUMERATE_SYSCALL(FCHMOD, 2)                   \
    __ENUMERATE_SYSCALL(GETPPID, 0)                  \
    __ENUMERATE_SYSCALL(SIGSUSPEND, 1)               \
    __ENUMERATE_SYSCALL(TIMES, 1)                    \
    __ENUMERATE_SYSCALL(CREATE_TASK, 6)              \
    __ENUMERATE_SYSCALL(EXIT_TASK, 0)                \
    __ENUMERATE_SYSCALL(UNUSED1, 0)                  \
    __ENUMERATE_SYSCALL(OS_MUTEX, 6)                 \
    __ENUMERATE_SYSCALL(TGKILL, 3)                   \
    __ENUMERATE_SYSCALL(GET_INITIAL_PROCESS_INFO, 1) \
    __ENUMERATE_SYSCALL(SET_THREAD_SELF_POINTER, 1)  \
    __ENUMERATE_SYSCALL(MPROTECT, 3)

enum sc_number {
#define __ENUMERATE_SYSCALL(x, v) SC_##x,
    SC_START,
    ENUMERATE_SYSCALLS SC_NUM
};

long __do_syscall(int sc, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6);
long syscall(enum sc_number sc, ...);
char *syscall_to_string(enum sc_number sc);

#endif /* _SYS_SYSCALL_H */