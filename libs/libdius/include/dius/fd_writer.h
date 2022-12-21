#pragma once

#include <di/prelude.h>

namespace dius {
class FdWriter {
public:
    constexpr explicit FdWriter(int fd) : m_fd(fd) {}

    di::Result<di::size_t> write(di::Span<di::Byte const>);
    di::Result<void> flush() { return {}; }

private:
    int m_fd { -1 };
};
}