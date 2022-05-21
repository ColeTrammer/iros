#pragma once

#include <liim/format.h>
#include <liim/hash_map.h>
#include <liim/new_vector.h>
#include <liim/result.h>
#include <liim/string.h>

#include "error.h"

namespace PortManager {
class Enviornment {
public:
    static Enviornment current();

    Enviornment set(String key, String value) &&;

    struct CStyleEnvp {
        NewVector<String> storage;
        NewVector<char*> envp;
    };
    CStyleEnvp get_c_style_envp();

private:
    explicit Enviornment(HashMap<String, String> enviornment);

    HashMap<String, String> m_enviornment;
};

class Process {
public:
    template<typename... Args>
    static Process command(Args&&... args) {
        NewVector<String> arguments;
        (arguments.push_back(format("{}", forward<Args>(args))), ...);
        return Process(move(arguments), None {});
    }
    static Process shell_command(String command);

    Process with_enviornment(Enviornment enviornment) &&;

    Result<Monostate, Error> spawn_and_wait();
    Result<pid_t, Error> spawn();
    Result<Monostate, Error> wait();

    String to_string() const;

private:
    explicit Process(NewVector<String> arguments, Option<Enviornment> enviornment);

    NewVector<String> m_arguments;
    Option<Enviornment> m_enviornment;
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
