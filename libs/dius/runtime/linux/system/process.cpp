#include "dius/system/process.h"

#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/wait.h>

#include "di/container/string/zstring.h"
#include "di/container/tree/tree_set.h"
#include "dius/linux/system_call.h"
#include "dius/print.h"
#include "dius/system/system_call.h"

namespace dius::system {
static auto s_envp = static_cast<char**>(nullptr);

[[gnu::constructor]] static void get_env_on_init(int, char**, char** envp) {
    s_envp = envp;
}

// I'm not sure why this isn't a u128.
using kernel_sigset_t = u64;

static auto sys_rt_sigprocmask(int how, kernel_sigset_t const* set, kernel_sigset_t* old) -> di::Result<void> {
    return system::system_call<int>(system::Number::rt_sigprocmask, how, set, old, sizeof(kernel_sigset_t)) %
           di::into_void;
}

static auto sys_rt_sigtimedwait(kernel_sigset_t const* set, void* info, void* timeout) -> di::Result<Signal> {
    return system::system_call<Signal>(system::Number::rt_sigtimedwait, set, info, timeout, sizeof(kernel_sigset_t));
}

static auto sys_kill(ProcessId id, int signal) -> di::Result<void> {
    return system::system_call<int>(system::Number::kill, id, signal) % di::into_void;
}

static auto sys_getpid() -> di::Result<ProcessId> {
    return system::system_call<ProcessId>(system::Number::getpid);
}

static auto file_open_mode_flags(OpenMode open_mode) -> int {
    switch (open_mode) {
        case OpenMode::Readonly:
            return O_RDONLY;
        case OpenMode::WriteNew:
            return O_WRONLY | O_EXCL | O_CREAT;
        case OpenMode::WriteClobber:
            return O_WRONLY | O_TRUNC | O_CREAT;
        case OpenMode::ReadWrite:
            return O_RDWR;
        case OpenMode::AppendOnly:
            return O_WRONLY | O_APPEND | O_CREAT;
        case OpenMode::ReadWriteClobber:
            return O_RDWR | O_TRUNC | O_CREAT;
        case OpenMode::AppendReadWrite:
            return O_RDWR | O_APPEND | O_CREAT;
        default:
            di::unreachable();
    }
}

auto ProcessHandle::self() -> ProcessHandle {
    // This really shouldn't fail...
    return ProcessHandle(sys_getpid().value());
}

auto ProcessHandle::wait() -> di::Result<ProcessResult> {
    if (id() == -1) {
        return di::Unexpected(di::BasicError::NoSuchProcess);
    }

    int status;
    TRY(system_call<ProcessId>(Number::wait4, id(), &status, 0, nullptr));

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

auto ProcessHandle::signal(Signal signal) -> di::Result<> {
    if (id() == -1) {
        return di::Unexpected(di::BasicError::NoSuchProcess);
    }
    return sys_kill(id(), int(signal));
}

static auto make_env(char** environ, di::TreeMap<di::TransparentString, di::TransparentString> const& extra_vars)
    -> di::Tuple<di::TreeSet<di::Box<di::TransparentString>>, di::Vector<char*>> {
    auto result = di::Vector<char*> {};
    for (auto* env_var = environ; *env_var; env_var++) {
        auto zstring = di::ZString(*env_var);
        auto it = di::find(zstring, '=');
        if (it == zstring.end()) {
            continue;
        }
        auto key = di::TransparentStringView(*env_var, &*it);
        if (extra_vars.contains(key)) {
            continue;
        }
        result.push_back(*env_var);
    }
    auto storage = di::TreeSet<di::Box<di::TransparentString>> {};
    for (auto const& [key, value] : extra_vars) {
        auto string = di::clone(key);
        string.push_back('=');
        string += value;
        auto [it, _] = storage.insert(di::make_box<di::TransparentString>(di::move(string)));
        result.push_back(const_cast<char*>((*it)->c_str()));
    }
    result.push_back(nullptr);
    return { di::move(storage), di::move(result) };
}

auto Process::spawn() && -> di::Result<ProcessHandle> {
    // NOTE: TransparentString objects are guaranteed to be null-terminated on Linux.
    auto null_terminated_args =
        di::concat(m_arguments | di::transform(di::cdata), di::single(nullptr)) | di::to<di::Vector>();
    auto [_, env] = make_env(s_envp, m_extra_env_vars);

    auto args = ::clone_args {
        .flags = CLONE_CLEAR_SIGHAND,
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
        // Any failures will cause the child process to exit with status 127.
        (void) ([&] -> di::Result<> {
            if (m_new_session) {
                TRY(system_call<int>(Number::setsid));
            }

            for (auto const& action : m_file_actions) {
                switch (action.type) {
                    case FileAction::Type::Open: {
                        // Always try to close the destination file descriptor.
                        (void) TRY(system_call<int>(Number::close, action.arg0));

                        auto flags = file_open_mode_flags(static_cast<OpenMode>(action.arg1));
                        auto fd =
                            TRY(system_call<int>(Number::openat, AT_FDCWD, action.path.c_str(), flags, action.arg2));
                        if (fd != action.arg0) {
                            TRY(system_call<int>(Number::dup2, fd, action.arg0));
                            TRY(system_call<int>(Number::close, fd));
                        }
                        break;
                    }
                    case FileAction::Type::Close: {
                        TRY(system_call<int>(Number::close, action.arg0));
                        break;
                    }
                    case FileAction::Type::Dup: {
                        TRY(system_call<int>(Number::dup2, action.arg0, action.arg1));
                        break;
                    }
                }
            }

            if (m_arguments[0].contains(U'/')) {
                TRY(system_call<int>(Number::execve, null_terminated_args[0], null_terminated_args.data(), env.data()));
            } else {
                // Read PATH env variable, and use it.
                auto path = "/bin:/usr/bin"_ts;
                for (auto const* const* env = s_envp; *env != nullptr; env++) {
                    auto zstring = di::ZCString(*env);
                    if (di::starts_with(zstring, "PATH="_tsv)) {
                        path = di::View(di::next(zstring.begin(), 5), zstring.end()) | di::transform([](char const& c) {
                                   return c;
                               }) |
                               di::to<di::TransparentString>();
                        break;
                    }
                }
                for (auto prefix : path | di::split(':')) {
                    auto absolute_program = prefix.to_owned();
                    if (!absolute_program.ends_with('/')) {
                        absolute_program.push_back('/');
                    }
                    absolute_program += m_arguments[0];
                    (void) system_call<int>(Number::execve, absolute_program.c_str(), null_terminated_args.data(),
                                            env.data());
                }
            }
            return {};
        }());
        exit_process(127);
    }

    return ProcessHandle(pid);
}

auto mask_signal(Signal signal) -> di::Result<void> {
    auto mask = kernel_sigset_t(1) << (kernel_sigset_t(signal) - 1);
    return sys_rt_sigprocmask(SIG_BLOCK, &mask, nullptr);
}

auto wait_for_signal(Signal signal) -> di::Result<Signal> {
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
