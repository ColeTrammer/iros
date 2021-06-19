#pragma once

#include <liim/byte_buffer.h>
#include <liim/function.h>
#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stdio.h>

namespace Ext {
class File {
public:
    static UniquePtr<File> create(const String& path, const String& type);

    explicit File(FILE* file);
    File(const File& other) = delete;
    File(File&& other) = delete;

    ~File();

    int error() const {
        int error = m_error;
        const_cast<File&>(*this).m_error = 0;
        return error;
    }

    Maybe<ByteBuffer> read_all();
    bool close();

    bool read(ByteBuffer& buffer);
    bool write(ByteBuffer& buffer);

    bool read_all_streamed(ByteBuffer& buffer, Function<bool(const ByteBuffer&)> callback);

private:
    FILE* m_file { nullptr };
    int m_error { 0 };
};
}
