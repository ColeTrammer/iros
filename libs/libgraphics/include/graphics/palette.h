#pragma once

#include <ext/mapped_file.h>
#include <graphics/color.h>

#define ENUMERATE_COLOR_TYPES                                         \
    __ENUMERATE_COLOR_TYPE(Background, background, ColorValue::Black) \
    __ENUMERATE_COLOR_TYPE(Text, text, ColorValue::White)             \
    __ENUMERATE_COLOR_TYPE(Outline, outline, ColorValue::White)

class Palette {
public:
    enum ColorType {
#define __ENUMERATE_COLOR_TYPE(t, l, d) t,
        ENUMERATE_COLOR_TYPES
#undef __ENUMERATE_COLOR_TYPE
            Count,
    };

    static SharedPtr<Palette> create_from_json(const String& path);

    Color color(ColorType type) const { return m_color_data[type]; }

    Palette(Vector<uint32_t> data) : m_colors(data.vector()), m_color_data(move(data)) {}

private:
    uint32_t* m_colors { nullptr };
    Vector<uint32_t> m_color_data;
};
