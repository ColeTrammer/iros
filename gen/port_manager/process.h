#pragma once

#include <liim/format.h>
#include <liim/new_vector.h>
#include <liim/result.h>
#include <liim/string.h>

#include "error.h"

namespace PortManager {
class Process {
public:
    template<typename... Args>
    static Process command(Args&&... args) {
        NewVector<String> arguments;
        (arguments.push_back(format("{}", forward<Args>(args))), ...);
        return Process(move(arguments));
    }
    static Process shell_command(String command);

    Result<Monostate, Error> spawn_and_wait();
    Result<pid_t, Error> spawn();
    Result<Monostate, Error> wait();

    String to_string() const;

private:
    explicit Process(NewVector<String> arguments);

    NewVector<String> m_arguments;
    Option<pid_t> m_pid;
};
}

namespace LIIM::Format {
template<>
struct Formatter<PortManager::Process> : public Formatter<String> {
    void format(const PortManager::Process& process, FormatContext& context) {
        return Formatter<String>::format(::format("{}", process.to_string()), context);
    }
};
}

