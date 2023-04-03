#pragma once

#include <di/prelude.h>
#include <dius/config.h>
#include <dius/error.h>

#include DIUS_PLATFORM_PATH(process.h)

namespace dius::system {
class ProcessResult {
public:
    explicit ProcessResult(int exit_code_or_signal, bool signaled)
        : m_exit_code_or_signal(exit_code_or_signal), m_signaled(signaled) {}

    bool signaled() const { return m_signaled; }
    bool exited() const { return !m_signaled; }

    int exit_code() const {
        ASSERT(exited());
        return m_exit_code_or_signal;
    }

    int signal() const {
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

    di::Result<ProcessResult> spawn_and_wait() &&;

private:
    di::Vector<di::TransparentString> m_arguments;
};

/// @brief Exit the currently executing thread.
///
/// @warning It is undefined behavior to call this function when there exists RAII stack-allocated variables
[[noreturn]] void exit_thread();

[[noreturn]] void exit_process(int status_code);
}
