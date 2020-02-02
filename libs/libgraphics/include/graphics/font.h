#pragma once

#include <fcntl.h>
#include <liim/bitmap.h>
#include <liim/hash_map.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static uint8_t font_unknown[16] = { 0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b10000010, 0b10000010, 0b10000010, 0b10000010,
                                    0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b11111110, 0b00000000, 0b00000000, 0b00000000 };

class Font {
public:
    Font() : m_unknown(Bitmap<uint8_t>::wrap(font_unknown, 16 * 8)) {
        int font_file = open("/usr/share/font.psf", O_RDONLY);
        assert(font_file != -1);

        uint8_t z[4];
        assert(read(font_file, z, 4) == 4);

        uint8_t b[16];
        int i = 0;
        while (read(font_file, b, 16) == 16) {
            auto bitmap = make_shared<Bitmap<uint8_t>>(16 * CHAR_BIT);
            memcpy(bitmap->bitmap(), b, 16);
            m_font_map.put(i++, bitmap);
        }

        close(font_file);
    }

    ~Font() {}

    SharedPtr<Bitmap<uint8_t>> get_for_character(int c) { return m_font_map.get_or(c, m_unknown); }

private:
    SharedPtr<Bitmap<uint8_t>> m_unknown;
    HashMap<int, SharedPtr<Bitmap<uint8_t>>> m_font_map;
};