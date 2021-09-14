#pragma once

#include <graphics/font.h>
#include <liim/bitset.h>
#include <liim/forward.h>
#include <liim/hash_map.h>

namespace PSF {
class Font : public ::Font {
public:
    static SharedPtr<Font> create_blank();
    static SharedPtr<Font> create_with_blank_characters(int num_chars);
    static SharedPtr<Font> try_create_from_path(const String& path);

    explicit Font(const String& path);
    explicit Font(int num_chars);
    virtual ~Font() override;

    virtual FontMetrics font_metrics() override;
    virtual Maybe<uint32_t> fallback_glyph_id() override;
    virtual Maybe<uint32_t> glyph_id_for_code_point(uint32_t code_point) override;
    virtual GlyphMetrics glyph_metrics(uint32_t glyph_id) override;
    virtual SharedPtr<Bitmap> rasterize_glyph(uint32_t glyph_id, Color color) override;

    bool save_to_file(const String& path);

    Bitset<uint8_t>& bitset_for_glyph_id(uint32_t id);

private:
    HashMap<uint32_t, Bitset<uint8_t>> m_font_map;
};
}
