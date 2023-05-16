#include <dius/system/process.h>
#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

namespace dius::system {
di::Result<ProcessResult> Process::spawn_and_wait() && {
    // NOTE: TransparentString objects are guaranteed to be null-terminated on Linux.
    auto null_terminated_args =
        di::concat(m_arguments | di::transform(di::cdata), di::single(nullptr)) | di::to<di::Vector>();

    auto pid = pid_t(-1);
    auto result = posix_spawnp(&pid, null_terminated_args[0], nullptr, nullptr,
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
