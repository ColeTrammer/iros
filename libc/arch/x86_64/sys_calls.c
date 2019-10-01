#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

void *sbrk(intptr_t increment) {
    void *ret;
    asm volatile( "movq $2, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(increment) : "rdi", "rsi", "rax");
    if (ret == (void*) -1) {
        errno = ENOMEM;
    }
    return ret;
}

__attribute__((__noreturn__))
void _exit(int status) {
    asm( "movq $1, %%rdi\n"\
         "movq %0, %%rsi\n"\
         "int $0x80" : : "m"(status) : "rdi", "rsi" );
    
    __builtin_unreachable();
}

int open(const char *pathname, int flags, mode_t mode) {
    int ret;
    asm volatile( "movq $4, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movl %2, %%edx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(flags), "r"(mode) : "rdi", "rsi", "edx", "ecx", "eax" );
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $5, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdi", "esi", "rdx", "rcx", "rax" );
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
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd) : "rdi", "esi", "eax" );
    __SYSCALL_TO_ERRNO(ret);
}

pid_t fork() {
    pid_t ret;
    asm volatile( "movq $3, %%rdi\n"\
                  "int $0x80\n"\
                  "mov %%eax, %0" : "=r"(ret) : : "rdi", "eax" );
    __SYSCALL_TO_ERRNO(ret);
}

int execve(const char *file, char *const argv[], char *const envp[]) {
    int ret;
    asm volatile( "movq $8, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(file), "r"(argv), "r"(envp) : "rdi", "rsi", "rdx", "rcx", "eax" );
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
                  "movq %%rax, %0" : "=r"(ret) : "r"(buf), "r"(size) : "rdi", "rsi", "rdx", "rax" );

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
                  "movl %%eax, %0" : "=r"(ret) : "r"(path) : "rdi", "rsi", "eax" );
    __SYSCALL_TO_ERRNO(ret);
}

int stat(const char *restrict path, struct stat *restrict stat_struct) {
    int ret;
    asm volatile( "movq $13, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "movq %2, %%rdx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(path), "r"(stat_struct) : "rdi", "rsi", "rdx", "eax" );
    __SYSCALL_TO_ERRNO(ret);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret;
    asm volatile( "movq $14, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movl %3, %%ecx\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(fd), "r"(offset), "r"(whence) : "rdi", "esi", "rdx", "ecx", "rax" );
    __SYSCALL_TO_ERRNO(ret);
}