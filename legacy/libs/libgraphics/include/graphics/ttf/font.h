#pragma once

#include <graphics/font.h>
#include <graphics/forward.h>
#include <liim/byte_buffer.h>
#include <liim/forward.h>

namespace TTF {
class Font : public ::Font {
public:
    static SharedPtr<Font> try_create_from_buffer(ByteBuffer buffer);

    virtual ~Font() override;

    virtual FontMetrics font_metrics() override;
    virtual Option<uint32_t> fallback_glyph_id() override;
    virtual Option<uint32_t> glyph_id_for_code_point(uint32_t code_point) override;
    virtual GlyphMetrics glyph_metrics(uint32_t glyph_id) override;
    virtual SharedPtr<Bitmap> rasterize_glyph(uint32_t glyph_id, Color color) override;

    uint32_t glyph_count() const;
    uint16_t number_of_h_metrics() const;

    const HeadTable& font_header_table() const { return m_font_header_table; }
    const HmtxTable& horizontal_metrics_table() const { return m_horizontal_metrics_table; }
    const MaxpTable& maximum_profile_table() const { return m_maximum_profile_table; }
    const HheaTable& horizontal_header_table() const { return m_horizontal_header_table; }
    const TableDirectory& table_directory() const { return m_table_directory; }
    const GlyphMapping& glyph_mapping() const { return *m_glyph_mapping; }

    const TableRecord* find_table(StringView tag) const;

    int font_units_to_pixels(int16_t value) {
        // FIXME: implement this.
        return value;
    }
    int font_units_to_pixels(uint16_t value) {
        // FIXME: implement this.
        return value;
    }

    const ByteBuffer& raw_buffer() const { return m_buffer; }

    explicit Font(ByteBuffer&& buffer, const TableDirectory& table_directory, const HeadTable& font_header_table,
                  const HheaTable& horizontal_header_table, const MaxpTable& maximum_profile_table,
                  const HmtxTable& horizontal_metrics_table, UniquePtr<GlyphMapping> glyph_mapping);

private:
    ByteBuffer m_buffer;
    const TableDirectory& m_table_directory;
    const HeadTable& m_font_header_table;
    const HheaTable& m_horizontal_header_table;
    const MaxpTable& m_maximum_profile_table;
    const HmtxTable& m_horizontal_metrics_table;
    UniquePtr<GlyphMapping> m_glyph_mapping;
};
}
