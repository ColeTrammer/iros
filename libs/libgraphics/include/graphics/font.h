#pragma once

#include <graphics/forward.h>
#include <liim/forward.h>
#include <stdint.h>

class Font {
public:
    static SharedPtr<Font> default_font();
    static SharedPtr<Font> bold_font();

    virtual ~Font();

    virtual Maybe<uint32_t> fallback_glyph_id() = 0;

    // FIXME: variation selectors/zero width join characters/ligatures make this API require more information.
    virtual Maybe<uint32_t> glyph_id_for_code_point(uint32_t code_point) = 0;

    // FIXME: this will depend on the font size being rendered, for true type fonts.
    // FIXME: font metrics are more complicated than just the width.
    virtual int width_of_glyph(uint32_t glyph_id) = 0;

    // FIXME: there's more to style than just color.
    // FIXME: ligatures may need to be rasterized together?
    // FIXME: there should be a atlas to cache the rasterized glyphs.
    // FIXME: some fonts have characters that don't require rasterization (emojis represented as images).
    virtual SharedPtr<Bitmap> rasterize_glyph(uint32_t glyph_id, Color color) = 0;

protected:
    Font();
};
