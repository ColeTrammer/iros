#include <graphics/psf/font.h>

#include <graphics/font.h>
#include <stdio.h>

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

const Bitset<uint8_t>* Font::get_for_character(int c) const {
    return m_font_map.get(c);
}

bool Font::save_to_file(const String& path) const {
    FILE* file = fopen(path.string(), "w");
    if (!file) {
        return false;
    }

    // PSF font header: 0x36 0x04 <filemode> <fontheight>
    fputc(0x36, file);
    fputc(0x04, file);
    fputc(0, file);
    fputc(16, file);

    for (int i = 0; i < 256; i++) {
        auto* bitset = get_for_character(i);
        if (!bitset) {
            break;
        }

        bitset->for_each_storage_part([&](uint8_t byte) {
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
