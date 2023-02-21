#include <di/prelude.h>
#include <dius/prelude.h>

#include <asm/prctl.h>

extern "C" {
extern void (*__preinit_array_start[])(int, char**, char**);
extern void (*__preinit_array_end[])(int, char**, char**);
extern void (*__init_array_start[])(int, char**, char**);
extern void (*__init_array_end[])(int, char**, char**);

extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
}

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

    iptr preinit_size = __preinit_array_end - __preinit_array_start;
    for (iptr i = 0; i < preinit_size; i++) {
        (*__preinit_array_start[i])(argc, argv, envp);
    }

    iptr init_size = __init_array_end - __init_array_start;
    for (iptr i = 0; i < init_size; i++) {
        (*__init_array_start[i])(argc, argv, envp);
    }

    _exit(__extension__ main(argc, argv, envp));
}
