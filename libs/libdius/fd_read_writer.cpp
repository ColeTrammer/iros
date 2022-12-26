#include <dius/fd_read_writer.h>
#include <unistd.h>

namespace dius {
di::Result<size_t> FdReadWriter::read(di::Span<di::Byte> data) const {
    auto result = ::read(m_fd, data.data(), data.size());
    (void) result;
    return di::to_unsigned(result);
}

di::Result<size_t> FdReadWriter::write(di::Span<di::Byte const> data) const {
    auto result = ::write(m_fd, data.data(), data.size());
    (void) result;
    return data.size();
}
}