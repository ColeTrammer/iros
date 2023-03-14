#include <dius/prelude.h>

#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/wait.h>

#ifndef DIUS_USE_RUNTIME
extern "C" [[noreturn]] void exit(int);
#endif

namespace dius::system {
static auto s_envp = static_cast<char**>(nullptr);

[[gnu::constructor]] static void get_env_on_init(int, char**, char** envp) {
    s_envp = envp;
}

di::Result<void> Process::swawn_and_wait() && {
    // NOTE: TransparentString objects are guaranteed to be null-terminated on Linux.
    auto null_terminated_args =
        di::concat(m_arguments | di::transform(di::cdata), di::single(nullptr)) | di::to<di::Vector>();

    auto args = ::clone_args {
        .flags = CLONE_VM | CLONE_FILES | CLONE_SIGHAND,
        .pidfd = 0,
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = SIGCHLD,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = 0,
        .set_tid_size = 0,
        .cgroup = 0,
    };

    auto pid = TRY(system_call<ProcessId>(Number::clone3, &args, sizeof(args)));

    // Child
    if (pid == 0) {
        // Now, any failures should result in a immediate call to exit.
        (void) system_call<int>(Number::execve, null_terminated_args[0], null_terminated_args.data(), s_envp);

        eprintln("Failed to exec: {}"_sv, m_arguments);

        exit_process(255);
    }

    // Parent
    int status;
    TRY(system_call<ProcessId>(Number::wait4, -1, &status, 0, nullptr));

    return {};
}

void exit_process(int code) {
#ifdef DIUS_USE_RUNTIME
    (void) dius::system::system_call<i32>(dius::system::Number::exit_group, code);
    di::unreachable();
#else
    exit(code);
#endif
}
}
