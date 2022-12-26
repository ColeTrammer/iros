#pragma once

#include <di/prelude.h>

namespace dius {
class FdReadWriter {
public:
    constexpr explicit FdReadWriter(int fd) : m_fd(fd) {}

    di::Result<di::size_t> read(di::Span<di::Byte>) const;

    di::Result<di::size_t> write(di::Span<di::Byte const>) const;
    di::Result<void> flush() const { return {}; }

private:
    int m_fd { -1 };
};

constexpr inline auto stdin = FdReadWriter { 0 };
constexpr inline auto stdout = FdReadWriter { 0 };
constexpr inline auto stderr = FdReadWriter { 0 };
}