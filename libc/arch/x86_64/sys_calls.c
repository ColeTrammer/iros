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

pid_t fork() {
    pid_t ret;
    asm volatile( "movq $3, %%rdi\n"\
                  "int $0x80\n"\
                  "mov %%eax, %0" : "=r"(ret) : : "rdi", "eax" );
    return ret;
}