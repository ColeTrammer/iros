#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1

#define ENUMERATE_SYSCALLS                                                     \
    __ENUMERATE_SYSCALL(EXIT, exit, 1)                                         \
    __ENUMERATE_SYSCALL(SBRK, sbrk, 1)                                         \
    __ENUMERATE_SYSCALL(FORK, fork, 0)                                         \
    __ENUMERATE_SYSCALL(OPEN, open, 3)                                         \
    __ENUMERATE_SYSCALL(READ, read, 3)                                         \
    __ENUMERATE_SYSCALL(WRITE, write, 3)                                       \
    __ENUMERATE_SYSCALL(CLOSE, close, 1)                                       \
    __ENUMERATE_SYSCALL(EXECVE, execve, 3)                                     \
    __ENUMERATE_SYSCALL(WAITPID, waitpid, 3)                                   \
    __ENUMERATE_SYSCALL(GETPID, getpid, 0)                                     \
    __ENUMERATE_SYSCALL(GETCWD, getcwd, 2)                                     \
    __ENUMERATE_SYSCALL(CHDIR, chdir, 1)                                       \
    __ENUMERATE_SYSCALL(STAT, stat, 2)                                         \
    __ENUMERATE_SYSCALL(LSEEK, lseek, 3)                                       \
    __ENUMERATE_SYSCALL(IOCTL, ioctl, 3)                                       \
    __ENUMERATE_SYSCALL(FTRUNCATE, ftruncate, 2)                               \
    __ENUMERATE_SYSCALL(GETTIMEOFDAY, gettimeofday, 3)                         \
    __ENUMERATE_SYSCALL(MKDIR, mkdir, 2)                                       \
    __ENUMERATE_SYSCALL(DUP2, dup2, 2)                                         \
    __ENUMERATE_SYSCALL(PIPE, pipe, 1)                                         \
    __ENUMERATE_SYSCALL(UNLINK, unlink, 1)                                     \
    __ENUMERATE_SYSCALL(RMDIR, rmdir, 1)                                       \
    __ENUMERATE_SYSCALL(CHMOD, chmod, 1)                                       \
    __ENUMERATE_SYSCALL(KILL, kill, 2)                                         \
    __ENUMERATE_SYSCALL(SETPGID, setpgid, 2)                                   \
    __ENUMERATE_SYSCALL(SIGACTION, sigaction, 3)                               \
    __ENUMERATE_SYSCALL(SIGRETURN, sigreturn, 0)                               \
    __ENUMERATE_SYSCALL(SIGPROCMASK, sigprocmask, 3)                           \
    __ENUMERATE_SYSCALL(DUP, dup, 1)                                           \
    __ENUMERATE_SYSCALL(GETPGID, getpgid, 1)                                   \
    __ENUMERATE_SYSCALL(SLEEP, sleep, 1)                                       \
    __ENUMERATE_SYSCALL(ACCESS, access, 2)                                     \
    __ENUMERATE_SYSCALL(ACCEPT4, accept4, 4)                                   \
    __ENUMERATE_SYSCALL(BIND, bind, 3)                                         \
    __ENUMERATE_SYSCALL(CONNECT, connect, 3)                                   \
    __ENUMERATE_SYSCALL(LISTEN, listen, 2)                                     \
    __ENUMERATE_SYSCALL(SOCKET, socket, 3)                                     \
    __ENUMERATE_SYSCALL(SHUTDOWN, shutdown, 2)                                 \
    __ENUMERATE_SYSCALL(GETSOCKOPT, getsockopt, 5)                             \
    __ENUMERATE_SYSCALL(SETSOCKOPT, setsockopt, 5)                             \
    __ENUMERATE_SYSCALL(GETPEERNAME, getpeername, 3)                           \
    __ENUMERATE_SYSCALL(GETSOCKNAME, getsockname, 3)                           \
    __ENUMERATE_SYSCALL(SENDTO, sendto, 6)                                     \
    __ENUMERATE_SYSCALL(RECVFROM, recvfrom, 6)                                 \
    __ENUMERATE_SYSCALL(MMAP, mmap, 6)                                         \
    __ENUMERATE_SYSCALL(MUNMAP, munmap, 2)                                     \
    __ENUMERATE_SYSCALL(RENAME, rename, 2)                                     \
    __ENUMERATE_SYSCALL(FCNTL, fcntl, 3)                                       \
    __ENUMERATE_SYSCALL(FSTAT, fstat, 2)                                       \
    __ENUMERATE_SYSCALL(ALARM, alarm, 1)                                       \
    __ENUMERATE_SYSCALL(FCHMOD, fchmod, 2)                                     \
    __ENUMERATE_SYSCALL(GETPPID, getppid, 0)                                   \
    __ENUMERATE_SYSCALL(SIGSUSPEND, sigsuspend, 1)                             \
    __ENUMERATE_SYSCALL(TIMES, times, 1)                                       \
    __ENUMERATE_SYSCALL(CREATE_TASK, create_task, 1)                           \
    __ENUMERATE_SYSCALL(EXIT_TASK, exit_task, 0)                               \
    __ENUMERATE_SYSCALL(OS_MUTEX, os_mutex, 6)                                 \
    __ENUMERATE_SYSCALL(TGKILL, tgkill, 3)                                     \
    __ENUMERATE_SYSCALL(GET_INITIAL_PROCESS_INFO, get_initial_process_info, 1) \
    __ENUMERATE_SYSCALL(SET_THREAD_SELF_POINTER, set_thread_self_pointer, 2)   \
    __ENUMERATE_SYSCALL(MPROTECT, mprotect, 3)                                 \
    __ENUMERATE_SYSCALL(SIGPENDING, sigpending, 1)                             \
    __ENUMERATE_SYSCALL(SIGALTSTACK, sigaltstack, 2)                           \
    __ENUMERATE_SYSCALL(PSELECT, pselect, 6)                                   \
    __ENUMERATE_SYSCALL(SETUID, setuid, 1)                                     \
    __ENUMERATE_SYSCALL(SETEUID, seteuid, 1)                                   \
    __ENUMERATE_SYSCALL(SETGID, setgid, 1)                                     \
    __ENUMERATE_SYSCALL(SETEGID, setegid, 1)                                   \
    __ENUMERATE_SYSCALL(GETUID, getuid, 0)                                     \
    __ENUMERATE_SYSCALL(GETEUID, geteuid, 0)                                   \
    __ENUMERATE_SYSCALL(GETGID, getgid, 0)                                     \
    __ENUMERATE_SYSCALL(GETEGID, getegid, 0)                                   \
    __ENUMERATE_SYSCALL(UMASK, umask, 1)                                       \
    __ENUMERATE_SYSCALL(UNAME, uname, 1)                                       \
    __ENUMERATE_SYSCALL(GETSID, getsid, 1)                                     \
    __ENUMERATE_SYSCALL(SETSID, setsid, 0)                                     \
    __ENUMERATE_SYSCALL(READLINK, readlink, 3)                                 \
    __ENUMERATE_SYSCALL(LSTAT, lstat, 2)

#ifdef __ASSEMBLER__
#define SC_SIGRETURN 27
#else

enum sc_number {
#define __ENUMERATE_SYSCALL(x, y, v) SC_##x,
    SC_START,
    ENUMERATE_SYSCALLS SC_NUM
};

__attribute__((__noreturn__)) void __sigreturn(void);
long __do_syscall(int sc, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6);
long syscall(enum sc_number sc, ...);
char *syscall_to_string(enum sc_number sc);

#endif /*__ASSEMBLER */

#endif /* _SYS_SYSCALL_H */