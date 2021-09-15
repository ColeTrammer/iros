#include <graphics/color.h>
#include <graphics/ttf/binary_format.h>
#include <graphics/ttf/font.h>
#include <liim/byte_io.h>

namespace TTF {
static const TableRecord* find_table_impl(const TableDirectory& directory, StringView tag) {
    for (uint32_t i = 0; i < directory.num_tables; i++) {
        auto& table_record = directory.table_records[i];
        if (tag_to_string_view(table_record.table_tag) == tag) {
            return &table_record;
        }
    }
    return nullptr;
}

Font::Font(ByteBuffer&& buffer, const TableDirectory& table_directory, const HeadTable& font_header_table,
           const HheaTable& horizontal_header_table, const MaxpTable& maximum_profile_table, const HmtxTable& horizontal_metrics_table)
    : m_buffer(move(buffer))
    , m_table_directory(table_directory)
    , m_font_header_table(font_header_table)
    , m_horizontal_header_table(horizontal_header_table)
    , m_maximum_profile_table(maximum_profile_table)
    , m_horizontal_metrics_table(horizontal_metrics_table) {}

Font::~Font() {}

SharedPtr<Font> Font::try_create_from_buffer(ByteBuffer buffer) {
    auto byte_reader = ByteReader { buffer.span() };

    auto* table_directory = byte_reader.pointer_at_offset<TableDirectory>(0);
    if (!table_directory) {
        return nullptr;
    }
    if (sizeof(*table_directory) + table_directory->num_tables * sizeof(table_directory->table_records[0]) > buffer.size()) {
        return nullptr;
    }

    auto* head_record = find_table_impl(*table_directory, "head");
    if (!head_record) {
        return nullptr;
    }

    auto* font_header_table = byte_reader.pointer_at_offset<HeadTable>(head_record->offset);
    if (!font_header_table) {
        return nullptr;
    }

    auto* hhea_record = find_table_impl(*table_directory, "hhea");
    if (!hhea_record) {
        return nullptr;
    }

    auto* horizontal_header_table = byte_reader.pointer_at_offset<HheaTable>(hhea_record->offset);
    if (!horizontal_header_table) {
        return nullptr;
    }

    auto* maxp_record = find_table_impl(*table_directory, "maxp");
    if (!maxp_record) {
        return nullptr;
    }

    auto* maximum_profile_table = byte_reader.pointer_at_offset<MaxpTable>(maxp_record->offset);
    if (!maximum_profile_table) {
        return nullptr;
    }

    auto glyph_count = maximum_profile_table->num_glyphs;
    auto number_of_h_metrics = horizontal_header_table->number_of_h_metrics;

    auto* hmtx_record = find_table_impl(*table_directory, "hmtx");
    if (!hmtx_record) {
        return nullptr;
    }

    auto* horizontal_metrics_table = byte_reader.pointer_at_offset<HmtxTable>(hmtx_record->offset);
    if (!horizontal_metrics_table) {
        return nullptr;
    }
    if (hmtx_record->offset + sizeof(uint16_t) * number_of_h_metrics + (glyph_count - number_of_h_metrics) > buffer.size()) {
        return nullptr;
    }

    return make_shared<Font>(move(buffer), *table_directory, *font_header_table, *horizontal_header_table, *maximum_profile_table,
                             *horizontal_metrics_table);
}

FontMetrics Font::font_metrics() {
    auto font_metrics = FontMetrics {};
    font_metrics.set_ascender(font_units_to_pixels(horizontal_header_table().ascender));
    font_metrics.set_descender(font_units_to_pixels(horizontal_header_table().descender));
    font_metrics.set_line_gap(font_units_to_pixels(horizontal_header_table().line_gap));
    return font_metrics;
}

Maybe<uint32_t> Font::fallback_glyph_id() {
    return 0;
}

Maybe<uint32_t> Font::glyph_id_for_code_point(uint32_t) {
    assert(false);
}

GlyphMetrics Font::glyph_metrics(uint32_t glyph_id) {
    auto glyph_metrics = GlyphMetrics {};
    auto& metrics_table = horizontal_metrics_table();
    if (glyph_id < number_of_h_metrics()) {
        glyph_metrics.set_advance_width(metrics_table.as_long_metrics()[glyph_id].advance_width);
        glyph_metrics.set_left_side_bearing(metrics_table.as_long_metrics()[glyph_id].lsb);
    } else {
        glyph_metrics.set_advance_width(metrics_table.as_long_metrics()[number_of_h_metrics() - 1].advance_width);
        glyph_metrics.set_left_side_bearing(
            metrics_table.as_short_metrics()[2 * number_of_h_metrics() + (glyph_id - number_of_h_metrics())]);
    }
    return glyph_metrics;
}

SharedPtr<Bitmap> Font::rasterize_glyph(uint32_t, Color) {
    assert(false);
}

uint32_t Font::glyph_count() const {
    return m_maximum_profile_table.num_glyphs;
}

uint16_t Font::number_of_h_metrics() const {
    return m_horizontal_header_table.number_of_h_metrics;
}

const TableRecord* Font::find_table(StringView tag) const {
    return find_table_impl(table_directory(), tag);
}
}
