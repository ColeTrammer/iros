#include "dius/system/process.h"

#include <csignal>
#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "di/container/algorithm/find.h"
#include "di/container/iterator/next.h"
#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/container/string/zstring.h"
#include "di/container/tree/tree_set.h"
#include "di/util/scope_exit.h"

namespace dius::system {
auto ProcessHandle::self() -> ProcessHandle {
    return ProcessHandle(getpid());
}

auto ProcessHandle::wait() -> di::Result<ProcessResult> {
    if (id() == -1) {
        return di::Unexpected(di::BasicError::NoSuchProcess);
    }

    auto status = 0;
    auto wait_result = waitpid(id(), &status, 0);
    if (wait_result == -1) {
        return di::Unexpected(di::BasicError(errno));
    }
    if (WIFEXITED(status)) {
        return ProcessResult { WEXITSTATUS(status), false };
    }
    return ProcessResult { WTERMSIG(status), true };
}

auto ProcessHandle::signal(Signal signal) -> di::Result<> {
    if (id() == -1) {
        return di::Unexpected(di::BasicError::NoSuchProcess);
    }

    auto res = kill(id(), int(signal));
    if (res < 0) {
        return di::Unexpected(di::BasicError(errno));
    }
    return {};
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

    auto file_actions = posix_spawn_file_actions_t {};
    posix_spawn_file_actions_init(&file_actions);
    auto file_action_cleanup = di::ScopeExit([&] {
        posix_spawn_file_actions_destroy(&file_actions);
    });
    for (auto const& action : m_file_actions) {
        switch (action.type) {
            case FileAction::Type::Open:
                posix_spawn_file_actions_addopen(&file_actions, action.arg0, action.path.c_str(),
                                                 file_open_mode_flags(static_cast<OpenMode>(action.arg1)), action.arg2);
                break;
            case FileAction::Type::Close:
                posix_spawn_file_actions_addclose(&file_actions, action.arg0);
                break;
            case FileAction::Type::Dup:
                posix_spawn_file_actions_adddup2(&file_actions, action.arg0, action.arg1);
                break;
            default:
                return di::Unexpected(di::BasicError::InvalidArgument);
        }
    }

    auto attrs = posix_spawnattr_t {};
    posix_spawnattr_init(&attrs);
    auto attrs_guard = di::ScopeExit([&] {
        posix_spawnattr_destroy(&attrs);
    });
    auto attr_flags = short(m_new_session ? POSIX_SPAWN_SETSID : 0);
    posix_spawnattr_setflags(&attrs, attr_flags);

    auto pid = pid_t(-1);
    auto [_, env] = make_env(environ, m_extra_env_vars);
    auto result = posix_spawnp(&pid, null_terminated_args[0], &file_actions, &attrs,
                               const_cast<char**>(null_terminated_args.data()), env.data());
    if (result != 0) {
        return di::Unexpected(di::BasicError(result));
    }
    return ProcessHandle(pid);
}

auto mask_signal(Signal signal) -> di::Result<void> {
    auto set = sigset_t {};
    sigemptyset(&set);
    sigaddset(&set, int(signal));
    int res = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (res < 0) {
        return di::Unexpected(di::BasicError(-res));
    }
    return {};
}

auto wait_for_signal(Signal signal) -> di::Result<Signal> {
    auto set = sigset_t {};
    sigemptyset(&set);
    sigaddset(&set, int(signal));
    int sig_out = 0;
    auto res = sigwait(&set, &sig_out);
    if (res < 0) {
        return di::Unexpected(di::BasicError(errno));
    }
    return Signal(sig_out);
}

void exit_thread() {
    pthread_exit(nullptr);
}

void exit_process(int code) {
    exit(code);
}
}
