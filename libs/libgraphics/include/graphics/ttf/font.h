#pragma once

#include <graphics/ttf/forward.h>
#include <liim/byte_buffer.h>
#include <liim/forward.h>

namespace TTF {
class Font {
public:
    static UniquePtr<Font> try_create_from_buffer(ByteBuffer buffer);

    ~Font();

    const TableDirectory& table_directory() const { return m_table_directory; }
    const TableRecord* find_table(StringView tag) const;

    const ByteBuffer& raw_buffer() const { return m_buffer; }

    explicit Font(ByteBuffer&& buffer, const TableDirectory& table_directory);

private:
    ByteBuffer m_buffer;
    const TableDirectory& m_table_directory;
};
}
