#include <graphics/ttf/binary_format.h>
#include <graphics/ttf/glyph_mapping.h>
#include <liim/byte_buffer.h>
#include <liim/byte_io.h>

namespace TTF {
UniquePtr<GlyphMapping> GlyphMapping::try_create(const ByteBuffer& buffer, const TableRecord& cmap_table_record) {
    auto byte_reader = ByteReader { buffer.span() };

    auto* character_to_glyph_index_table = byte_reader.pointer_at_offset<CmapTable>(cmap_table_record.offset);
    if (!character_to_glyph_index_table) {
        return nullptr;
    }
    if (character_to_glyph_index_table->minimum_size() > buffer.size()) {
        return nullptr;
    }

    for (uint32_t i = 0; i < character_to_glyph_index_table->num_tables; i++) {
        auto& encoding_record = character_to_glyph_index_table->encoding_records[i];
        if (encoding_record.platform_id == EncodingRecord::PlatformId::Unicocde &&
            encoding_record.platform_specific_id == EncodingRecord::UnicodeEncodingID::Unicode2_0BMP) {
            auto* format = byte_reader.pointer_at_offset<BigEndian<uint16_t>>(cmap_table_record.offset + encoding_record.offset);
            if (!format) {
                return nullptr;
            }
            if (*format == 4) {
                if (auto glyph_mapping_format4 = GlyphMappingFormat4::try_create(buffer, cmap_table_record, encoding_record)) {
                    return glyph_mapping_format4;
                }
            }
        }
    }
    return nullptr;
}

UniquePtr<GlyphMappingFormat4> GlyphMappingFormat4::try_create(const ByteBuffer& buffer, const TableRecord& cmap_table_record,
                                                               const EncodingRecord& encoding_record) {
    auto byte_reader = ByteReader { buffer.span() };

    auto* format4_table = byte_reader.pointer_at_offset<CmapSubtable4>(cmap_table_record.offset + encoding_record.offset);
    if (!format4_table) {
        return nullptr;
    }

    // FIXME: validate the length of the table.
    return make_unique<GlyphMappingFormat4>(*format4_table);
}

Option<uint16_t> GlyphMappingFormat4::lookup_code_point(uint32_t code_point) {
    for (uint16_t i = 0; i < table().segment_count(); i++) {
        auto segment_end = table().end_character_code(i);
        if (segment_end < code_point) {
            continue;
        }

        auto segment_start = table().start_character_code(i);
        if (code_point < segment_start) {
            return {};
        }

        auto delta = table().segment_delta(i);
        auto range_offset = table().segment_range_offset(i);
        if (range_offset == 0) {
            return (delta + code_point) % 65536U;
        }

        // FIXME: validate that the offset in bounds.
        uint16_t glyph_offset = *(range_offset / sizeof(uint16_t) + (code_point - segment_start) + &table().segment_range_offset(i));
        if (glyph_offset == 0) {
            return {};
        }
        return glyph_offset + delta;
    }
    return {};
}
}
