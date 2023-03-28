#include <dius/prelude.h>
#include <linux/sched.h>

namespace dius {
struct [[gnu::aligned(16)]] StackHead {
    void (*entry)(StackHead*);
    void* closure;
};

[[gnu::naked]] static long do_spawn_thread(::clone_args*, usize) {
    // Do the system call, and then move the new stack pointer into the 1st argument passed to the entry. The general
    // idea is that we put the new thread entry point onto its new stack, so that when execute the `ret` instruction,
    // the processor will go there. But for the calling thread, the function will return normally.
    asm volatile("syscall\n"
                 "mov %%rsp, %%rdi\n"
                 "ret\n"
                 :
                 : "a"(di::to_underlying(system::Number::clone3))
                 : DIUS_SYSTEM_CALL_CLOBBER);
}

di::Result<void> spawn_thread(PlatformThread& platform_thread) {
    auto* stack_data = platform_thread.stack.span().typed_pointer_unchecked<StackHead>(platform_thread.stack.size() -
                                                                                       sizeof(StackHead));
    stack_data->closure = static_cast<void*>(&platform_thread.entry);
    stack_data->entry = [](StackHead* stack) {
        auto* as_function = reinterpret_cast<di::Function<void()>*>(stack->closure);
        (*as_function)();
        system::exit_thread();
    };

    auto args = ::clone_args {
        .flags = CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_SYSVSEM | CLONE_THREAD | CLONE_VM |
                 CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | CLONE_SETTLS,
        .pidfd = 0,
        .child_tid = di::to_uintptr(&platform_thread.thread_id),
        .parent_tid = 0,
        .exit_signal = 0,
        .stack = di::to_uintptr(platform_thread.stack.data()),
        .stack_size = platform_thread.stack.size() - sizeof(StackHead),
        .tls = di::to_uintptr(&platform_thread),
        .set_tid = 0,
        .set_tid_size = 0,
        .cgroup = usize(-1),
    };
    auto result = do_spawn_thread(&args, sizeof(args));
    if (result < 0) {
        return di::Unexpected(PosixError(-result));
    }
    return {};
}
}
