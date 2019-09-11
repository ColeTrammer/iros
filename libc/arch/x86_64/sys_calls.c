#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void *sbrk(intptr_t increment) {
    void *ret;
    asm volatile( "movq $2, %%rdi\n"\
                  "movq %1, %%rsi\n"\
                  "int $0x80\n"\
                  "movq %%rax, %0" : "=r"(ret) : "r"(increment) : "rdi", "rsi", "rax");
    return ret;
}

bool sys_print(void *buffer, size_t n) {
    bool ret;
	asm volatile( "movq $0, %%rdi\n"\
	              "movq %1, %%rsi\n"\
	              "movq %2, %%rdx\n"\
	              "int $0x80\n"\
	              "movb %%al, %0" : "=r"(ret) : "r"(buffer), "r"(n) : "rdi", "rsi", "rdx", "rax" );
	return ret;
}

int open(const char *pathname, int flags, mode_t mode) {
    int ret;
    asm volatile( "movq $4, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(pathname), "r"(flags), "r"(mode) : "rdx", "rsi", "rdx", "rcx", "eax" );
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $5, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdx", "rsi", "rdx", "rcx", "eax" );
    return ret;
}

ssize_t write(int fd, const void * buf, size_t count) {
    ssize_t ret;
    asm volatile( "movq $6, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "movq %2, %%rdx\n"\
                  "movq %3, %%rcx\n"\
                  "int $0x80\n"\
                  "movl %%rax, %0" : "=r"(ret) : "r"(fd), "r"(buf), "r"(count) : "rdx", "rsi", "rdx", "rcx", "eax" );
    return ret;
}

int close(int fd) {
    int ret;
    asm volatile( "movq $7, %%rdi\n"\
                  "movl %1, %%esi\n"\
                  "int $0x80\n"\
                  "movl %%eax, %0" : "=r"(ret) : "r"(fd) : "rdi", "rsi", "eax" );
    return ret;
}

pid_t fork() {
    pid_t ret;
    asm volatile( "movq $3, %%rdi\n"\
                  "int $0x80\n"\
                  "mov %%eax, %0" : "=r"(ret) : : "rdi", "eax" );
    return ret;
}