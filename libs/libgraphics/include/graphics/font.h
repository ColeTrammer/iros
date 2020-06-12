#pragma once

#include <fcntl.h>
#include <liim/bitmap.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static uint8_t font_unknown[16] = { 0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b10000010, 0b10000010, 0b10000010, 0b10000010,
                                    0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b11111110, 0b00000000, 0b00000000, 0b00000000 };

class Font {
public:
    static const Font& default_font() {
        static Font* s_default;
        if (!s_default) {
            s_default = new Font("/usr/share/font.psf");
        }
        return *s_default;
    }

    static const Font& bold_font() {
        static Font* s_bold;
        if (!s_bold) {
            s_bold = new Font("/usr/share/bold.psf");
        }
        return *s_bold;
    }

    Font(const char* path) : m_unknown(Bitmap<uint8_t>::wrap(font_unknown, 16 * 8)) {
        int font_file = open(path, O_RDONLY);
        assert(font_file != -1);

        uint8_t z[4];
        assert(read(font_file, z, 4) == 4);

        uint8_t b[16];
        int i = 0;
        while (read(font_file, b, 16) == 16) {
            auto bitmap = make_shared<Bitmap<uint8_t>>(16 * 8);
            memcpy(bitmap->bitmap(), b, 16);
            m_font_map.put(i++, bitmap);
        }

        close(font_file);
    }

    ~Font() {}

    SharedPtr<Bitmap<uint8_t>> get_for_character(int c) const { return m_font_map.get_or(c, m_unknown); }

    bool save_to_file(const String& path) const;

private:
    SharedPtr<Bitmap<uint8_t>> m_unknown;
    HashMap<int, SharedPtr<Bitmap<uint8_t>>> m_font_map;
};
