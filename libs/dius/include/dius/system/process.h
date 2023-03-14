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

[[noreturn]] void exit_process(int status_code);
}
