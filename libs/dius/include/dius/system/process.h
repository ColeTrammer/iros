#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/vector/prelude.h>
#include <di/util/prelude.h>
#include <dius/config.h>
#include <dius/error.h>

#include DIUS_PLATFORM_PATH(process.h)

namespace dius::system {
class ProcessResult {
public:
    explicit ProcessResult(int exit_code_or_signal, bool signaled)
        : m_exit_code_or_signal(exit_code_or_signal), m_signaled(signaled) {}

    auto signaled() const -> bool { return m_signaled; }
    auto exited() const -> bool { return !m_signaled; }

    auto exit_code() const -> int {
        ASSERT(exited());
        return m_exit_code_or_signal;
    }

    auto signal() const -> int {
        ASSERT(signaled());
        return m_exit_code_or_signal;
    }

private:
    int m_exit_code_or_signal { 0 };
    bool m_signaled { false };
};

class Process {
public:
    explicit Process(di::Vector<di::TransparentString> arguments) : m_arguments(di::move(arguments)) {}

    auto spawn_and_wait() && -> di::Result<ProcessResult>;

private:
    di::Vector<di::TransparentString> m_arguments;
};

/// @brief Exit the currently executing thread.
///
/// @warning It is undefined behavior to call this function when there exists RAII stack-allocated variables
[[noreturn]] void exit_thread();

[[noreturn]] void exit_process(int status_code);
}
