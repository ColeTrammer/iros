#define __libc_internal

#include <assert.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <utime.h>

void *sbrk(intptr_t increment) {
    void *ret;
    asm volatile( "movq $2, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(increment) : "rdi", "rsi", "rax", "memory" );
    if (ret == (void*) -1) {
        errno = ENOMEM;
    }
    return ret;
}

__attribute__((__noreturn__))
void _exit(int status) {
    asm( "movq $1, %%rdi\n"\
         "movq %0, %%rsi\n"\
         "int $0x80" : : "m"(status) : "rdi", "rsi", "memory" );
    
    __builtin_unreachable();
}

int open(const char *pathname, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags); 
    mode_t mode = flags & O_CREAT ? va_arg(parameters, mode_t) : 0;
    int ret;
    asm volatile( "movq $4, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(flags), "r"(mode) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $5, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $6, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int close(int fd) {
    int ret;
    asm volatile( "movq $7, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t fork() {
    pid_t ret;
    asm volatile( "movq $3, %%rdi\n"\
                  "int $0x80\n"\
                  "mov %%eax, %0" : "=r"(ret) : : "rdi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int execve(const char *file, char *const argv[], char *const envp[]) {
    int ret;
    asm volatile( "movq $8, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(file), "r"(argv), "r"(envp) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {
    pid_t ret;
    asm volatile( "movq $9, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid), "r"(wstatus), "r"(options) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpid() {
    pid_t ret;
    asm volatile( "movq $10, %%rdi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : : "rdi", "rax" );
    return ret;
}

char *getcwd(char *buf, size_t size) {
    if (buf == NULL) {
        if (size == 0) {
            size = 4096;
        }
        buf = malloc(size);
    }

    char *ret;
    asm volatile( "movq $11, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(buf), "r"(size) : "rdi", "rsi", "rdx", "rax", "memory" );

    if (ret == NULL) {
        errno = ERANGE;
    }

    return ret;
}

int chdir(const char *path) {
    int ret;
    asm volatile( "movq $12, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int stat(const char *restrict path, struct stat *restrict stat_struct) {
    int ret;
    asm volatile( "movq $13, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(stat_struct) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret;
    asm volatile( "movq $14, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(offset), "r"(whence) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int ioctl(int fd, unsigned long request, ...) {
    va_list parameters;
    va_start(parameters, request); 
    void *argp = va_arg(parameters, void*);

    int ret;
    asm volatile( "movq $15, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(request), "r"(argp) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    
    va_end(parameters);
    __SYSCALL_TO_ERRNO(ret);
}

int ftruncate(int fd, off_t length) {
    int ret;
    asm volatile( "movq $16, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(length) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int gettimeofday(struct timeval *__restrict tv, void *__restrict tz) {
    int ret;
    asm volatile( "movq $17, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(tv), "r"(tz) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int mkdir(const char *path, mode_t mode) {
    int ret;
    asm volatile( "movq $18, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(mode) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int dup2(int oldfd, int newfd) {
    int ret;
    asm volatile( "movq $19, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r" (oldfd), "r"(newfd) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int pipe(int pipefd[2]) {
    int ret;
    asm volatile( "movq $20, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pipefd) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int unlink(const char *pathname) {
    int ret;
    asm volatile( "movq $21, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int rmdir(const char *pathname) {
    int ret;
    asm volatile( "movq $22, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int chmod(const char *pathname, mode_t mode) {
    int ret;
    asm volatile( "movq $23, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(mode) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int kill(pid_t pid, int sig) {
    int ret;
    asm volatile( "movq $24, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid), "r"(sig) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t setpgid(pid_t pid, pid_t pgid) {
    int ret;
    asm volatile( "movq $25, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid), "r"(pgid) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *old_act) {
    int ret;
    ((struct sigaction*) act)->sa_restorer = &sigreturn;
    asm volatile( "movq $26, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(signum), "r"(act), "r"(old_act) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

__attribute__((noreturn))
void sigreturn() {
    asm volatile( "movq $27, %%rdi\n"\
                  "int $0x80" : : : "rdi", "memory" );
    __builtin_unreachable();
}

int sigprocmask(int how, const sigset_t *set, sigset_t *old) {
    int ret;
    asm volatile( "movq $28, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(how), "r"(set), "r"(old) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

uid_t getuid(void) {
    return 0;
}

uid_t geteuid(void) {
    return 0;
}

gid_t getgid(void) {
    return 0;
}

gid_t getegid(void) {
    return 0;
}

int setuid(uid_t uid) {
    (void) uid;
    return 0;
}

int setgid(gid_t gid) {
    (void) gid;
    return 0;
}

int seteuid(uid_t uid) {
    (void) uid;
    return 0;
}

int setegid(gid_t gid) {
    (void) gid;
    return 0;
}

int dup(int oldfd) {
    int ret;
    asm volatile( "movq $29, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(oldfd) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpgid(pid_t pid) {
    pid_t ret;
    asm volatile( "movq $30, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

unsigned int sleep(unsigned int seconds) {
    unsigned int ret;
    asm volatile( "movq $31, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(seconds) : "rdi", "rsi", "rax", "memory" );
    return ret;
}

int access(const char *path, int mode) {
    int ret;
    asm volatile( "movq $32, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(mode) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int accept4(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen, int flags) {
    int ret;
    asm volatile( "movq $33, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "movl %4, %%r8d\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(addr), "r"(addrlen), "r"(flags) : "rdi", "rsi", "rdx", "rcx", "r8", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret;
    asm volatile( "movq $34, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(addr), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret;
    asm volatile( "movq $35, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(addr), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int listen(int fd, int backlog) {
    int ret;
    asm volatile( "movq $36, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(backlog) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int socket(int domain, int type, int protocol) {
    int ret;
    asm volatile( "movq $37, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(domain), "r"(type), "r"(protocol) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int shutdown(int fd, int how) {
    int ret;
    asm volatile( "movq $38, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(how) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int getsockopt(int fd, int level, int optname, void *__restrict optval, socklen_t *__restrict optlen) {
    int ret;
    asm volatile( "movq $39, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "movq %4, %%r8\n"\
                  "movq %5, %%r9\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(level), "r"(optname), "r"(optval), "r"(optlen) : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    int ret;
    asm volatile( "movq $40, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "movq %4, %%r8\n"\
                  "movl %5, %%r9d\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(level), "r"(optname), "r"(optval), "r"(optlen) : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int getpeername(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    int ret;
    asm volatile( "movq $41, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(addr), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int getsockname(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    int ret;
    asm volatile( "movq $42, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(addr), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen) {
    ssize_t ret;
    asm volatile( "movq $43, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "movl %4, %%r8d\n"\
                  "movq %5, %%r9\n"\
                  "movl %6, %%r10d\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(len), "r"(flags), "r"(dest), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *__restrict source, socklen_t *__restrict addrlen) {
    ssize_t ret;
    asm volatile( "movq $44, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "movl %4, %%r8d\n"\
                  "movq %5, %%r9\n"\
                  "movq %6, %%r10\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(len), "r"(flags), "r"(source), "r"(addrlen) : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void *ret;
    asm volatile( "movq $45, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "movl %4, %%r8d\n"\
                  "movl %5, %%r9d\n"\
                  "movq %6, %%r10\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(addr), "r"(length), "r"(prot), "r"(flags), "r"(fd), "r"(offset) : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "rax", "memory" );
    if ((long) ret < 0) {
        errno = -((long) ret);
        return MAP_FAILED;
    }

    return ret;
}

int munmap(void *addr, size_t length) {
    int ret;
    asm volatile( "movq $46, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(addr), "r"(length) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int rename(const char *old, const char *new_path) {
    int ret;
    asm volatile( "movq $47, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(old), "r"(new_path) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int fcntl(int fd, int command, ...) {
    int ret;
    va_list args;
    va_start(args, command);
    int arg = command == F_GETFD || command == F_GETFL ? 0 : va_arg(args, int);
    asm volatile( "movq $48, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(command), "r"(arg) : "rdi", "rsi", "rdx", "rcx", "rax", "memory" );
    va_end(args);
    __SYSCALL_TO_ERRNO(ret);
}

int fstat(int fd, struct stat *stat_struct) {
    int ret;
    asm volatile( "movq $49, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(stat_struct) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

unsigned int alarm(unsigned int seconds) {
    unsigned int ret;
    asm volatile( "movq $50, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(seconds) : "rdi", "rsi", "rax", "memory" );
    return ret;
}

int fchmod(int fd, mode_t mode) {
    int ret;
    asm volatile( "movq $51, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(mode) : "rdi", "rsi", "rdx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getppid(void) {
    pid_t ret;
    asm volatile( "movq $52, %%rdi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : : "rdi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

mode_t umask(mode_t mask) {
    (void) mask;

    fprintf(stderr, "umask not supported\n");
    return 0;
}

int mknod(const char *path, mode_t mode, dev_t dev) {
    (void) path;
    (void) mode;
    (void) dev;

    fprintf(stderr, "mknod not supported\n");
    assert(false);
    return 0;
}

clock_t clock(void) {
    fprintf(stderr, "clock not supported\n");

    assert(false);
    return 0;
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    (void) pathname;
    (void) owner;
    (void) group;

    fprintf(stderr, "chown not supported\n");
    return 0;
}

int utime(const char *filename, const struct utimbuf *times) {
    (void) filename;
    (void) times;

    fprintf(stderr, "utime not supported\n");
    return 0;
}

int sigsuspend(const sigset_t *set) {
    int ret;
    asm volatile( "movq $53, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(set) : "rdi", "rsi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

clock_t times(struct tms *buf) {
    clock_t ret;
    asm volatile( "movq $54, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(buf) : "rdi", "rsi", "rax", "memory" );

    // times is special since the clock_t can overflow to a negative number
    if (ret == (clock_t) -EFAULT) {
        errno = EFAULT;
        return -1;
    }
    return ret;
}