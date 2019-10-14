#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>

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
    mode_t mode = va_arg(parameters, mode_t);
    int ret;
    asm volatile( "movq $4, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(flags), "r"(mode) : "rdi", "rsi", "edx", "ecx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $5, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdi", "esi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $6, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdi", "esi", "rdx", "rcx", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int close(int fd) {
    int ret;
    asm volatile( "movq $7, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd) : "rdi", "esi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t fork() {
    pid_t ret;
    asm volatile( "movq $3, %%rdi\n"\
                  "int $0x80\n"\
                  "mov %%eax, %0" : "=r"(ret) : : "rdi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int execve(const char *file, char *const argv[], char *const envp[]) {
    int ret;
    asm volatile( "movq $8, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(file), "r"(argv), "r"(envp) : "rdi", "rsi", "rdx", "rcx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {
    pid_t ret;
    asm volatile( "movq $9, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid), "r"(wstatus), "r"(options) : "rdi", "esi", "rdx", "ecx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t getpid() {
    pid_t ret;
    asm volatile( "movq $10, %%rdi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : : "rdi", "eax" );
    return ret;
}

char *getcwd(char *buf, size_t size) {
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
                  "movl %%eax, %0" : "=r"(ret) : "r"(path) : "rdi", "rsi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int stat(const char *restrict path, struct stat *restrict stat_struct) {
    int ret;
    asm volatile( "movq $13, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(stat_struct) : "rdi", "rsi", "rdx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret;
    asm volatile( "movq $14, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(offset), "r"(whence) : "rdi", "esi", "rdx", "ecx", "rax", "memory" );
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
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(request), "r"(argp) : "rdi", "esi", "rdx", "rcx", "eax", "memory" );
    
    va_end(parameters);
    __SYSCALL_TO_ERRNO(ret);
}

int ftruncate(int fd, off_t length) {
    int ret;
    asm volatile( "movq $16, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd), "r"(length) : "rdi", "esi", "rdx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

time_t get_time() {
    time_t ret;
    asm volatile( "movq $17, %%rdi\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : : "rdi", "rax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int mkdir(const char *path, mode_t mode) {
    int ret;
    asm volatile( "movq $18, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(mode) : "rdi", "rsi", "edx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int dup2(int oldfd, int newfd) {
    int ret;
    asm volatile( "movq $19, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r" (oldfd), "r"(newfd) : "rdi", "esi", "edx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int pipe(int pipefd[2]) {
    int ret;
    asm volatile( "movq $20, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pipefd) : "rdi", "rsi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int unlink(const char *pathname) {
    int ret;
    asm volatile( "movq $21, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname) : "rdi", "rsi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int rmdir(const char *pathname) {
    int ret;
    asm volatile( "movq $22, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname) : "rdi", "rsi", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int chmod(const char *pathname, mode_t mode) {
    int ret;
    asm volatile( "movq $23, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(mode) : "rdi", "rsi", "edx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
}

int kill(pid_t pid, int sig) {
    int ret;
    asm volatile( "movq $24, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movl %2, %%edx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pid), "r"(sig) : "rdi", "esi", "edx", "eax", "memory" );
    __SYSCALL_TO_ERRNO(ret);
} 