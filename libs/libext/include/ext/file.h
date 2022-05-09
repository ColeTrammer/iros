#pragma once

#include <liim/byte_buffer.h>
#include <liim/format.h>
#include <liim/forward.h>
#include <liim/option.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stdio.h>

namespace Ext {
enum class StripTrailingNewlines { Yes, No };

class File {
public:
    static UniquePtr<File> create(const String& path, const String& type);

    explicit File(FILE* file);
    File(const File& other) = delete;
    File(File&& other) = delete;

    ~File();

    int fd() const { return m_file ? fileno(m_file) : -1; }
    FILE* c_file() const { return m_file; }

    void set_should_close_file(bool b) { m_should_close_file = b; }
    int error() const {
        int error = m_error;
        const_cast<File&>(*this).m_error = 0;
        return error;
    }

    Option<ByteBuffer> read_all();
    bool read_all_lines(Function<bool(String)>, StripTrailingNewlines strip_newlines);
    bool close();

    bool read(ByteBuffer& buffer);
    bool write(ByteBuffer& buffer);
    bool write(StringView view);

    bool read_all_streamed(ByteBuffer& buffer, Function<bool(const ByteBuffer&)> callback);

    template<typename... Args>
    bool writef(StringView view, const Args&... args) {
        auto string = vformat(view, make_format_args(args...));
        return write(string.view());
    }

private:
    FILE* m_file { nullptr };
    int m_error { 0 };
    bool m_should_close_file { true };
};
}
