#include "dius/system/process.h"

#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "di/util/scope_exit.h"

namespace dius::system {
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

auto Process::spawn_and_wait() && -> di::Result<ProcessResult> {
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
    auto attr_flags = m_new_session ? POSIX_SPAWN_SETSID : 0;
    posix_spawnattr_setflags(&attrs, attr_flags);

    auto pid = pid_t(-1);
    auto result = posix_spawnp(&pid, null_terminated_args[0], &file_actions, &attrs,
                               const_cast<char**>(null_terminated_args.data()), environ);
    if (result != 0) {
        return di::Unexpected(di::BasicError(result));
    }

    auto status = 0;
    auto wait_result = waitpid(pid, &status, 0);
    if (wait_result == -1) {
        return di::Unexpected(di::BasicError(errno));
    }
    if (WIFEXITED(status)) {
        return ProcessResult { WEXITSTATUS(status), false };
    }
    return ProcessResult { WTERMSIG(status), true };
}

void exit_thread() {
    pthread_exit(nullptr);
}

void exit_process(int code) {
    exit(code);
}
}
