#include <sys/syscall.h>

#include "loader.h"

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

void *sbrk(intptr_t increment) {
    void *ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%rsi\n"
                 "int $0x80\n"
                 "mov %%rax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_SBRK), "r"(increment)
                 : "rdi", "rsi", "rax", "memory");
    return ret;
}

int os_mutex(unsigned int *__protected, int op, int expected, int to_place, int to_wake, unsigned int *to_wait) {
    int ret;
    asm volatile("mov %1, %%rdi\n"
                 "mov %2, %%rsi\n"
                 "mov %3, %%edx\n"
                 "mov %4, %%ecx\n"
                 "mov %5, %%r8d\n"
                 "mov %6, %%r9d\n"
                 "mov %7, %%r10\n"
                 "int $0x80\n"
                 "mov %%eax, %0\n"
                 : "=r"(ret)
                 : "i"(SC_OS_MUTEX), "r"(__protected), "r"(op), "r"(expected), "r"(to_place), "r"(to_wake), "r"(to_wait)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "rax", "memory");
    return ret;
}

#include "../libc/bits/lock/__lock.c"
#include "../libc/bits/lock/__unlock.c"
#include "../libc/string/memcmp.c"
#include "../libc/string/memcpy.c"
#include "../libc/string/memmove.c"
#include "../libc/string/memset.c"
#include "../libc/string/strchr.c"
#include "../libc/string/strcmp.c"
#include "../libc/string/strlen.c"
