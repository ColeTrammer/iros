#include <fcntl.h>
#include <sys/syscall.h>

#include "../../loader.h"

void _exit(int status) {
    asm volatile("mov %0, %%rdi\n"
                 "mov %1, %%esi\n"
                 "int $0x80\n"
                 :
                 : "i"(SC_EXIT), "r"(status)
                 : "rdi", "rsi", "memory");
    __builtin_unreachable();
}

ssize_t write(int fd, const void *buffer, size_t len) {
    ssize_t ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%esi\n"
                 "mov %3, %%rdx\n"
                 "mov %4, %%rcx\n"
                 "int $0x80\n"
                 "mov %%rax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_WRITE), "r"(fd), "r"(buffer), "r"(len)
                 : "rdi", "rsi", "rdx", "rcx", "rax", "memory");
    return ret;
}

int open(const char *path, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags);
    int ret;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        mode = va_arg(parameters, mode_t);
    }
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%esi\n"
                 "mov %3, %%rdx\n"
                 "mov %4, %%ecx\n"
                 "mov %5, %%r8d\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_OPENAT), "i"(AT_FDCWD), "r"(path), "r"(flags), "r"(mode)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "rax", "memory");
    va_end(parameters);
    return ret;
}

int fstat(int fd, struct stat *st) {
    int ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%esi\n"
                 "mov %3, %%rdx\n"
                 "mov %4, %%rcx\n"
                 "mov %5, %%r8d\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_FSTATAT), "r"(fd), "r"(""), "r"(st), "i"(AT_EMPTY_PATH)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "rax", "memory");
    return ret;
}

int close(int fd) {
    int ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%esi\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_CLOSE), "r"(fd)
                 : "rdi", "rsi", "rax", "memory");
    return ret;
}

void *mmap(void *addr, size_t size, int prot, int flags, int fd, off_t offset) {
    void *ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%rsi\n"
                 "mov %3, %%rdx\n"
                 "mov %4, %%ecx\n"
                 "mov %5, %%r8d\n"
                 "mov %6, %%r9d\n"
                 "mov %7, %%r10\n"
                 "int $0x80\n"
                 "mov %%rax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_MMAP), "r"(addr), "r"(size), "r"(prot), "r"(flags), "r"(fd), "r"(offset)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "rax", "memory");
    return ret;
}

int mprotect(void *base, size_t size, int prot) {
    int ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%rsi\n"
                 "mov %3, %%rdx\n"
                 "mov %4, %%ecx\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_MPROTECT), "r"(base), "r"(size), "r"(prot)
                 : "rdi", "rsi", "rdx", "rcx", "rax", "memory");
    return ret;
}

int munmap(void *base, size_t size) {
    int ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%rsi\n"
                 "mov %3, %%rdx\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_MUNMAP), "r"(base), "r"(size)
                 : "rdi", "rsi", "rdx", "rax", "memory");
    return ret;
}
