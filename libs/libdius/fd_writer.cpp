#include <dius/fd_writer.h>
#include <unistd.h>

namespace dius {
di::Result<size_t> FdWriter::write(di::Span<di::Byte const> data) {
    auto result = ::write(m_fd, data.data(), data.size());
    (void) result;
    return data.size();
}
}