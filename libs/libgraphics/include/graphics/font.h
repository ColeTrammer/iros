#pragma once

#include <graphics/forward.h>
#include <liim/forward.h>
#include <liim/utilities.h>
#include <stdint.h>

class FontMetrics {
public:
    FontMetrics() = default;

    int ascender() const { return m_ascender; }
    int descender() const { return m_descender; }
    int line_gap() const { return m_line_gap; }

    void set_ascender(int ascender) { m_ascender = ascender; }
    void set_descender(int descender) { m_descender = descender; }
    void set_line_gap(int line_gap) { m_line_gap = line_gap; }

    int line_height() const { return ascender() + descender() + line_gap(); }

private:
    int m_ascender { 0 };
    int m_descender { 0 };
    int m_line_gap { 0 };
};

class GlyphMetrics {
public:
    GlyphMetrics() = default;

    int left_side_bearing() const { return m_left_side_bearing; }
    int advance_width() const { return m_advance_width; }

    void set_left_side_bearing(int left_side_bearing) { m_left_side_bearing = left_side_bearing; }
    void set_advance_width(int advance_width) { m_advance_width = advance_width; }

private:
    int m_left_side_bearing { 0 };
    int m_advance_width { 0 };
};

class Glyph {
public:
    Glyph(uint32_t glyph_id, GlyphMetrics metrics) : m_id { glyph_id }, m_metrics { move(metrics) } {}

    uint32_t id() const { return m_id; }
    const GlyphMetrics& metrics() const { return m_metrics; }

private:
    uint32_t m_id { 0 };
    GlyphMetrics m_metrics;
};

class Font {
public:
    static SharedPtr<Font> default_font();
    static SharedPtr<Font> bold_font();

    virtual ~Font();

    virtual FontMetrics font_metrics() = 0;

    virtual Option<uint32_t> fallback_glyph_id() = 0;

    // FIXME: variation selectors/zero width join characters/ligatures make this API require more information.
    virtual Option<uint32_t> glyph_id_for_code_point(uint32_t code_point) = 0;

    // FIXME: this will depend on the font size being rendered, for true type fonts.
    virtual GlyphMetrics glyph_metrics(uint32_t glyph_id) = 0;

    // FIXME: there's more to style than just color.
    // FIXME: ligatures may need to be rasterized together?
    // FIXME: there should be a atlas to cache the rasterized glyphs.
    // FIXME: some fonts have characters that don't require rasterization (emojis represented as images).
    virtual SharedPtr<Bitmap> rasterize_glyph(uint32_t glyph_id, Color color) = 0;

protected:
    Font();
};
