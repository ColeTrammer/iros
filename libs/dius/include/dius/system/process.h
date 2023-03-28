#pragma once

#include <dius/config.h>
#include <dius/error.h>

#include DIUS_PLATFORM_PATH(process.h)

namespace dius::system {
class Process {
public:
    explicit Process(di::Vector<di::TransparentString> arguments) : m_arguments(di::move(arguments)) {}

    di::Result<void> swawn_and_wait() &&;

private:
    di::Vector<di::TransparentString> m_arguments;
};

/// @brief Exit the currently executing thread.
///
/// @warning It is undefined behavior to call this function when there exists RAII stack-allocated variables
[[noreturn]] void exit_thread();

[[noreturn]] void exit_process(int status_code);
}
