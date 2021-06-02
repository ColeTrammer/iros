#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    stack_t stack;
    stack.ss_flags = 0;
    stack.ss_size = SIGSTKSZ;
    stack.ss_sp = malloc(stack.ss_size);
    fprintf(stderr, "allocated stack: %p\n", stack.ss_sp);

    if (sigaltstack(&stack, nullptr)) {
        perror("sigaltstack");
    }

    struct sigaction act;
    act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = [](int, siginfo_t*, void* _context [[maybe_unused]]) {
        uint64_t rsp;
        asm("mov %%rsp, %0" : "=r"(rsp) : :);
        fprintf(stderr, "\n%%rsp=%#.16lX\n", rsp);
#ifdef __os_2__
        ucontext_t* context = reinterpret_cast<ucontext_t*>(_context);
        fprintf(stderr, "rsp on now: [ %p ]\n", context->uc_stack.ss_sp);
        fprintf(stderr, "rsp return: [ %#.16lX ]\n", context->uc_mcontext.__stack_state.rsp);
#endif /* __os_2__ */
    };

    sigaction(SIGINT, &act, nullptr);

    pause();

    fputs("done\n", stderr);
    return 0;
}
