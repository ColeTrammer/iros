#include "dius/system/process.h"

#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/wait.h>

#include "dius/linux/system_call.h"
#include "dius/print.h"
#include "dius/system/system_call.h"

namespace dius::system {
static auto s_envp = static_cast<char**>(nullptr);

[[gnu::constructor]] static void get_env_on_init(int, char**, char** envp) {
    s_envp = envp;
}

auto Process::spawn_and_wait() && -> di::Result<ProcessResult> {
    // NOTE: TransparentString objects are guaranteed to be null-terminated on Linux.
    auto null_terminated_args =
        di::concat(m_arguments | di::transform(di::cdata), di::single(nullptr)) | di::to<di::Vector>();

    auto args = ::clone_args {
        .flags = 0,
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

    // NOTE: Linux's wait.h header does not define WIFEXITED, WEXITSTATUS, WIFSIGNALED, and WTERMSIG, so it is done
    //       manually here. In the future, it would be nice to take these definitions from libccpp's headers.
    auto const signal = (status & 0x7F);
    if (signal == 0) {
        // Exited.
        return ProcessResult { (status & 0xFF00) >> 8, false };
    }
    // Signaled.
    return ProcessResult { (status & 0x7F), true };
}

using kernel_sigset_t = u128;

static di::Result<void> sys_rt_sigprocmask(int how, kernel_sigset_t const* set, kernel_sigset_t* old) {
    return system::system_call<int>(system::Number::rt_sigprocmask, how, set, old, sizeof(kernel_sigset_t)) %
           di::into_void;
}

static di::Result<Signal> sys_rt_sigtimedwait(kernel_sigset_t const* set, void* info, void* timeout) {
    return system::system_call<Signal>(system::Number::rt_sigtimedwait, set, info, timeout, sizeof(kernel_sigset_t));
}

static di::Result<void> sys_kill(ProcessId id, int signal) {
    return system::system_call<int>(system::Number::kill, id, signal) % di::into_void;
}

static di::Result<ProcessId> sys_getpid() {
    return system::system_call<ProcessId>(system::Number::getpid);
}

ProcessId get_process_id() {
    // This really shouldn't fail...
    return sys_getpid().value();
}

di::Result<void> mask_signal(Signal signal) {
    auto mask = kernel_sigset_t(1) << (kernel_sigset_t(signal) - 1);
    return sys_rt_sigprocmask(SIG_BLOCK, &mask, nullptr);
}

di::Result<void> send_signal(ProcessId id, Signal signal) {
    return sys_kill(id, int(signal));
}

di::Result<Signal> wait_for_signal(Signal signal) {
    auto mask = kernel_sigset_t(1) << (kernel_sigset_t(signal) - 1);
    return sys_rt_sigtimedwait(&mask, nullptr, nullptr);
}

void exit_thread() {
    (void) dius::system::system_call<i32>(dius::system::Number::exit, 0);
    di::unreachable();
}

void exit_process(int code) {
    (void) dius::system::system_call<i32>(dius::system::Number::exit_group, code);
    di::unreachable();
}
}
