#include <graphics/font.h>
#include <stdio.h>

FontImpl::FontImpl(const char* path) {
    int font_file = open(path, O_RDONLY);
    assert(font_file != -1);

    uint8_t z[4];
    assert(read(font_file, z, 4) == 4);

    uint8_t b[16];
    int i = 0;
    while (read(font_file, b, 16) == 16) {
        auto bitmap = Bitmap<uint8_t>(16 * 8);
        memcpy(bitmap.bitmap(), b, 16);
        m_font_map.put(i++, move(bitmap));
    }

    close(font_file);
}

FontImpl::FontImpl(int num_chars) {
    for (int i = 0; i < num_chars; i++) {
        auto empty_bitmap = Bitmap<uint8_t>(16 * 8);
        memset(empty_bitmap.bitmap(), 0, 16);
        m_font_map.put(i, move(empty_bitmap));
    }
}

FontImpl::~FontImpl() {}

const Bitmap<uint8_t>* FontImpl::get_for_character(int c) const {
    return m_font_map.get(c);
}

bool FontImpl::save_to_file(const String& path) const {
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
        auto* bitmap = get_for_character(i);
        if (!bitmap) {
            break;
        }

        bitmap->for_each_storage_part([&](uint8_t byte) {
            fputc(byte, file);
        });
    }

    bool ret = !ferror(file);

    if (fclose(file)) {
        return false;
    }

    return ret;
}

Font Font::create_blank() {
    return Font(256);
}
