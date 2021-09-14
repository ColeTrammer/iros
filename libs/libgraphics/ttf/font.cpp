#include <graphics/ttf/binary_format.h>
#include <graphics/ttf/font.h>
#include <liim/byte_io.h>

namespace TTF {
Font::Font(ByteBuffer&& buffer, const TableDirectory& table_directory) : m_buffer(move(buffer)), m_table_directory(table_directory) {}

Font::~Font() {}

UniquePtr<Font> Font::try_create_from_buffer(ByteBuffer buffer) {
    auto byte_reader = ByteReader { buffer.span() };

    auto* table_directory = byte_reader.pointer_at_offset<TableDirectory>(0);
    if (!table_directory) {
        return nullptr;
    }
    if (sizeof(*table_directory) + table_directory->num_tables * sizeof(table_directory->table_records[0]) > buffer.size()) {
        return nullptr;
    }

    return make_unique<Font>(move(buffer), *table_directory);
}

const TableRecord* Font::find_table(StringView tag) const {
    auto& directory = table_directory();
    for (uint32_t i = 0; i < directory.num_tables; i++) {
        auto& table_record = directory.table_records[i];
        if (tag_to_string_view(table_record.table_tag) == tag) {
            return &table_record;
        }
    }
    return nullptr;
}
}
