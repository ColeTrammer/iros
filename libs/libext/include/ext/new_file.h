#pragma once

#include <ext/system.h>
#include <liim/error/system_domain.h>
#include <liim/forward.h>
#include <stdio.h>
#include <sys/types.h>

namespace Ext {
class NewFile {
public:
    enum class OpenMode { Readonly, WriteNew, WriteClobber, ReadWrite, AppendOnly };
    static System::Result<NewFile> create(StringView path, OpenMode open_mode, mode_t create_mode = 0666);

    NewFile(NewFile&& other) : m_file(exchange(other.m_file, nullptr)) {}

    ~NewFile();

    bool is_open() const { return !!m_file; }
    Option<int> fd() const;

    System::Result<void> write(const ByteBuffer& buffer);
    System::Result<void> write(StringView buffer);
    System::Result<void> write(Span<const uint8_t> buffer);

    System::Result<size_t> write_allow_partial(const ByteBuffer& buffer);
    System::Result<size_t> write_allow_partial(StringView buffer);
    System::Result<size_t> write_allow_partial(Span<const uint8_t> buffer);

    System::Result<void> close();

    System::Result<ByteBuffer> map(System::Protection protection = System::Protection::Readable,
                                   System::MapFlags flags = System::MapFlags::Shared) const;

private:
    explicit NewFile(FILE* file);

    FILE* m_file { nullptr };
};
}
