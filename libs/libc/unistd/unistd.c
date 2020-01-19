#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

void *sbrk(intptr_t increment) {
    void *ret = (void *) syscall(SC_SBRK, increment);
    if (ret == (void *) -1) {
        errno = ENOMEM;
    }
    return ret;
}

__attribute__((__noreturn__)) void _exit(int status) {
    syscall(SC_EXIT, status);
    __builtin_unreachable();
}

pid_t fork() {
    pid_t ret = (pid_t) syscall(SC_FORK);
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret = syscall(SC_READ, fd, buf, count);
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret = syscall(SC_WRITE, fd, buf, count);
    __SYSCALL_TO_ERRNO(ret);
}

int close(int fd) {
    int ret = (int) syscall(SC_CLOSE, fd);
    __SYSCALL_TO_ERRNO(ret);
}

int execve(const char *file, char *const argv[], char *const envp[]) {
    int ret = (int) syscall(SC_EXECVE, file, argv, envp);
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpid() {
    return (pid_t) syscall(SC_GETPID);
}

char *getcwd(char *buf, size_t size) {
    if (buf == NULL) {
        if (size == 0) {
            size = 4096;
        }
        buf = malloc(size);
    }

    char *ret = (char *) syscall(SC_GETCWD, buf, size);
    if (ret == NULL) {
        errno = ERANGE;
    }

    return ret;
}

int chdir(const char *path) {
    int ret = (int) syscall(SC_CHDIR, path);
    __SYSCALL_TO_ERRNO(ret);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret = syscall(SC_LSEEK, fd, offset, whence);
    __SYSCALL_TO_ERRNO(ret);
}

int ftruncate(int fd, off_t length) {
    int ret = (int) syscall(SC_FTRUNCATE, fd, length);
    __SYSCALL_TO_ERRNO(ret);
}

int dup2(int oldfd, int newfd) {
    int ret = (int) syscall(SC_DUP2, oldfd, newfd);
    __SYSCALL_TO_ERRNO(ret);
}

int pipe(int pipefd[2]) {
    int ret = (int) syscall(SC_PIPE, pipefd);
    __SYSCALL_TO_ERRNO(ret);
}

int unlink(const char *pathname) {
    int ret = (int) syscall(SC_UNLINK, pathname);
    __SYSCALL_TO_ERRNO(ret);
}

int rmdir(const char *pathname) {
    int ret = (int) syscall(SC_RMDIR, pathname);
    __SYSCALL_TO_ERRNO(ret);
}

pid_t setpgid(pid_t pid, pid_t pgid) {
    pid_t ret = (pid_t) syscall(SC_SETPGID, pid, pgid);
    __SYSCALL_TO_ERRNO(ret);
}

uid_t getuid(void) {
    return (uid_t) syscall(SC_GETUID);
}

uid_t geteuid(void) {
    return (uid_t) syscall(SC_GETEUID);
}

gid_t getgid(void) {
    return (gid_t) syscall(SC_GETGID);
}

gid_t getegid(void) {
    return (gid_t) syscall(SC_GETEGID);
}

pid_t getsid(pid_t pid) {
    pid_t ret = (pid_t) syscall(SC_GETSID, pid);
    __SYSCALL_TO_ERRNO(ret);
}

int setuid(uid_t uid) {
    int ret = (int) syscall(SC_SETUID, uid);
    __SYSCALL_TO_ERRNO(ret);
}

int setgid(gid_t gid) {
    int ret = (int) syscall(SC_SETGID, gid);
    __SYSCALL_TO_ERRNO(ret);
}

int seteuid(uid_t uid) {
    int ret = (int) syscall(SC_SETEUID, uid);
    __SYSCALL_TO_ERRNO(ret);
}

int setegid(gid_t gid) {
    int ret = (int) syscall(SC_SETEGID, gid);
    __SYSCALL_TO_ERRNO(ret);
}

pid_t setsid(void) {
    pid_t ret = (pid_t) syscall(SC_SETSID);
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpgrp(void) {
    return getpgid(0);
}

int dup(int oldfd) {
    int ret = (int) syscall(SC_DUP, oldfd);
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpgid(pid_t pid) {
    pid_t ret = (pid_t) syscall(SC_GETPGID, pid);
    __SYSCALL_TO_ERRNO(ret);
}

unsigned int sleep(unsigned int seconds) {
    return (unsigned int) syscall(SC_SLEEP, seconds);
}

int access(const char *path, int mode) {
    int ret = (int) syscall(SC_ACCESS, path, mode);
    __SYSCALL_TO_ERRNO(ret);
}

int rename(const char *old, const char *new_path) {
    int ret = (int) syscall(SC_RENAME, old, new_path);
    __SYSCALL_TO_ERRNO(ret);
}

unsigned int alarm(unsigned int seconds) {
    return (unsigned int) syscall(SC_ALARM, seconds);
}

pid_t getppid(void) {
    pid_t ret = (pid_t) syscall(SC_GETPID);
    __SYSCALL_TO_ERRNO(ret);
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    (void) pathname;
    (void) owner;
    (void) group;

    fprintf(stderr, "chown not supported\n");
    return 0;
}

int lstat(const char *__restrict path, struct stat *__restrict stat_struct) {
    int ret = (int) syscall(SC_LSTAT, path, stat_struct);
    __SYSCALL_TO_ERRNO(ret);
}

int symlink(const char *target, const char *linkpath) {
    int ret = (int) syscall(SC_SYMLINK, target, linkpath);
    __SYSCALL_TO_ERRNO(ret);
}

int link(const char *newpath, const char *oldpath) {
    int ret = (int) syscall(SC_LINK, newpath, oldpath);
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t readlink(const char *__restrict pathname, char *__restrict buf, size_t bufsiz) {
    ssize_t ret = (ssize_t) syscall(SC_READLINK, pathname, buf, bufsiz);
    __SYSCALL_TO_ERRNO(ret);
}

int pause(void) {
    sigset_t set;
    sigprocmask(SIG_SETMASK, NULL, &set);
    return sigsuspend(&set);
}

int mknod(const char *path, mode_t mode, dev_t dev) {
    (void) path;
    (void) mode;
    (void) dev;

    fprintf(stderr, "mknod not supported\n");
    assert(false);
    return 0;
}

int sysconf(int name) {
    switch (name) {
        case _SC_PAGE_SIZE:
            return PAGE_SIZE;
        default:
            errno = EINVAL;
            return 0;
    }
}
