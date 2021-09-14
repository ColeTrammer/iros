#include <assert.h>
#include <fcntl.h>
#include <graphics/bitmap.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/psf/font.h>
#include <liim/string.h>
#include <stdio.h>
#include <unistd.h>

namespace PSF {
SharedPtr<Font> Font::create_blank() {
    return Font::create_with_blank_characters(256);
}

SharedPtr<Font> Font::create_with_blank_characters(int num_chars) {
    return make_shared<Font>(num_chars);
}

SharedPtr<Font> Font::try_create_from_path(const String& path) {
    // FIXME: this should be fallible.
    return make_shared<Font>(path);
}

Font::Font(const String& path) {
    int font_file = open(path.string(), O_RDONLY);
    assert(font_file != -1);

    uint8_t z[4];
    assert(read(font_file, z, 4) == 4);

    uint8_t b[16];
    int i = 0;
    while (read(font_file, b, 16) == 16) {
        auto bitset = Bitset<uint8_t>(16 * 8);
        memcpy(bitset.bitset(), b, 16);
        m_font_map.put(i++, move(bitset));
    }

    close(font_file);
}

Font::Font(int num_chars) {
    for (int i = 0; i < num_chars; i++) {
        auto empty_bitset = Bitset<uint8_t>(16 * 8);
        memset(empty_bitset.bitset(), 0, 16);
        m_font_map.put(i, move(empty_bitset));
    }
}

Font::~Font() {}

Maybe<uint32_t> Font::fallback_glyph_id() {
    return glyph_id_for_code_point('?');
}

Maybe<uint32_t> Font::glyph_id_for_code_point(uint32_t code_point) {
    if (code_point < 256) {
        return code_point;
    }
    return {};
}

int Font::width_of_glyph(uint32_t) {
    return 8;
}

SharedPtr<Bitmap> Font::rasterize_glyph(uint32_t glyph_id, Color color) {
    auto bitmap = make_shared<Bitmap>(8, 16, true);
    auto& bitset = bitset_for_glyph_id(glyph_id);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 8; x++) {
            auto bit = bitset.get(y * 8 + (7 - x));
            bitmap->put_pixel(x, y, bit ? color : ColorValue::Clear);
        }
    }
    return move(bitmap);
}

Bitset<uint8_t>& Font::bitset_for_glyph_id(uint32_t glyph_id) {
    return *m_font_map.get(glyph_id);
}

bool Font::save_to_file(const String& path) {
    FILE* file = fopen(path.string(), "w");
    if (!file) {
        return false;
    }

    // PSF font header: 0x36 0x04 <filemode> <fontheight>
    fputc(0x36, file);
    fputc(0x04, file);
    fputc(0, file);
    fputc(16, file);

    for (uint32_t i = 0; i < 256; i++) {
        bitset_for_glyph_id(i).for_each_storage_part([&](uint8_t byte) {
            fputc(byte, file);
        });
    }

    bool ret = !ferror(file);

    if (fclose(file)) {
        return false;
    }

    return ret;
}
}
