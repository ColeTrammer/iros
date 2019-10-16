#include <signal.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

__attribute__((noreturn))
void on_int(int sig) {
    assert(sig == SIGINT);

    write(1, "INTERRUPT\n", 10);
    asm volatile("add $8, %%rsp\n"\
                 "movq $27, %%rdi\n"\
                 "int $0x80" : : : "rdi", "memory" );

    __builtin_unreachable();
}

int main() {
    struct sigaction to_set;
    to_set.sa_flags = 0;
    to_set.sa_handler = &on_int;
    sigaction(SIGINT, &to_set, NULL);
    char c;
    while (1) { read(0, &c, 1); };
}