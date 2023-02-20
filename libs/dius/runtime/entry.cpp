#include <di/prelude.h>
#include <dius/prelude.h>

#include <asm/prctl.h>

extern "C" int main(int, char**, char**);

extern "C" [[noreturn]] [[gnu::naked]] void _start() {
    asm volatile("xor %rbp, %rbp\n"
                 "mov (%rsp), %edi\n"
                 "lea 8(%rsp), %rsi\n"
                 "lea 16(%rsp ,%rdi ,8), %rdx\n"
                 "call dius_entry\n");
}

extern "C" [[noreturn]] void _exit(int code) {
    (void) dius::system::system_call<i32>(dius::system::Number::exit_group, code);
    di::unreachable();
}

static char buffer[4096];

extern "C" void dius_entry(int argc, char** argv, char** envp) {
    (void) dius::system::system_call<i32>(dius::system::Number::arch_prctl, ARCH_SET_FS, buffer + 4096, 0, 0, 0, 0);
    _exit(__extension__ main(argc, argv, envp));
}
