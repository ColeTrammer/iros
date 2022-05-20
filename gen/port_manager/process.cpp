#include <liim/try.h>
#include <spawn.h>
#include <sys/wait.h>

#include "process.h"

namespace PortManager {
Process Process::shell_command(String command) {
    return Process::command("sh", "-c", move(command));
}

Process::Process(NewVector<String> arguments) : m_arguments(move(arguments)) {}

Result<Monostate, Error> Process::spawn_and_wait() {
    m_pid = TRY(spawn());
    return wait();
}

Result<pid_t, Error> Process::spawn() {
    NewVector<char*> arguments;
    for (auto& argument : m_arguments) {
        arguments.push_back(argument.string());
    }
    arguments.push_back(nullptr);

    pid_t pid;
    int result = posix_spawnp(&pid, m_arguments[0].string(), nullptr, nullptr, arguments.data(), environ);
    if (result != 0) {
        return Err(Ext::StringError(format("Failed to spawn process \"{}\": {}", *this, strerror(result))));
    }
    return Ok(pid);
}

Result<Monostate, Error> Process::wait() {
    assert(m_pid);
    int status;
    pid_t result = waitpid(*m_pid, &status, 0);
    if (result < 0) {
        return Err(Ext::StringError(format("Failed to wait on pid '{}': {}", *m_pid, strerror(errno))));
    } else if (result != *m_pid) {
        return Err(Ext::StringError(format("Waited on wrong pid: got {}, but expected {}", result, *m_pid)));
    }

    if (WIFSIGNALED(status)) {
        return Err(Ext::StringError(format("Process \"{}\" was sent fatal signal: {}", *this, strsignal(WTERMSIG(status)))));
    }
    assert(WIFEXITED(status));
    if (WEXITSTATUS(status) != 0) {
        return Err(Ext::StringError(format("Process \"{}\" terminated with non-zero exit code {}", *this, WEXITSTATUS(status))));
    }

    m_pid.reset();
    return Ok(Monostate {});
}

String Process::to_string() const {
    auto result = ""s;
    bool first = true;
    for (auto& argument : m_arguments) {
        if (!first) {
            result += " ";
        }
        result += argument;
        first = false;
    }
    return result;
}
}
