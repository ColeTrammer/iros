#include <liim/try.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process.h"

namespace PortManager {
Enviornment Enviornment::current() {
    HashMap<String, String> enviornment;
    for (auto** pair = environ; *pair; pair++) {
        auto key_equal_value = String(*pair);
        if (auto equal_index = key_equal_value.index_of('=')) {
            enviornment.put(key_equal_value.first(*equal_index), key_equal_value.substring(*equal_index + 1));
        }
    }
    return Enviornment(move(enviornment));
}

Result<Enviornment, Error> Enviornment::from_json(const JsonReader&, const Ext::Json::Object& object) {
    auto result = current();
    object.for_each([&](auto& key, auto& value) {
        result = move(result).set(key, Ext::Json::stringify(value));
    });
    return Ok(result);
}

Enviornment::Enviornment(HashMap<String, String> enviornment) : m_enviornment(move(enviornment)) {}

Enviornment Enviornment::set(String key, String value) && {
    m_enviornment.put(move(key), move(value));
    return Enviornment(move(m_enviornment));
}

auto Enviornment::get_c_style_envp() -> CStyleEnvp {
    NewVector<String> storage;
    m_enviornment.for_each_key([&](auto& key) {
        storage.push_back(format("{}={}", key, *m_enviornment.get(key)));
    });
    NewVector<char*> envp;
    for (auto& value : storage) {
        envp.push_back(value.string());
    }
    envp.push_back(nullptr);
    return { move(storage), move(envp) };
}

Process Process::shell_command(String command) {
    return Process::command("sh", "-c", move(command));
}

Process Process::from_arguments(NewVector<String> arguments) {
    return Process(move(arguments), {});
}

Process::Process(NewVector<String> arguments, Option<Enviornment> enviornment)
    : m_arguments(move(arguments)), m_enviornment(move(enviornment)) {}

Process Process::with_enviornment(Enviornment enviornment) && {
    assert(!m_pid);
    return Process(move(m_arguments), move(enviornment));
}

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
    int result;
    if (m_enviornment) {
        auto c_enviornment = m_enviornment->get_c_style_envp();
        result = posix_spawnp(&pid, m_arguments[0].string(), nullptr, nullptr, arguments.data(), c_enviornment.envp.data());
    } else {
        result = posix_spawnp(&pid, m_arguments[0].string(), nullptr, nullptr, arguments.data(), environ);
    }
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
