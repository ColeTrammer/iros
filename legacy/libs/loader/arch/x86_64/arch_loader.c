#include "../../loader.h"

__attribute__((noreturn)) void loader_exec(struct initial_process_info *info, int argc, char **argv, char **envp) {
#ifdef LOADER_DEBUG
    loader_log("starting program");
#endif /* LOADER_DEBUG */
    ran_program = 1;
    asm volatile("and $(~15), %%rsp\n"
                 "sub $8, %%rsp\n"
                 "mov %0, %%rdi\n"
                 "mov %1, %%esi\n"
                 "mov %2, %%rdx\n"
                 "mov %3, %%rcx\n"
                 "jmp *%4\n"
                 :
                 : "r"(info), "r"(argc), "r"(argv), "r"(envp), "r"(info->program_entry)
                 : "rdi", "rsi", "rdx", "rcx", "memory");
    _exit(99);
}
