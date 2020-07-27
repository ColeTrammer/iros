#include <sys/syscall.h>

extern "C" {
void _entry() {
    asm volatile("mov %0, %%rdi\n"
                 "mov $42, %%rsi\n"
                 "int $0x80\n"
                 :
                 : "i"(SC_EXIT)
                 : "rdi", "rsi", "memory");
}
}
