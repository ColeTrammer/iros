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
    static SharedPtr<Palette> create_from_shared_memory(const String& path, int prot);

    static constexpr size_t byte_size() { return sizeof(uint32_t) * ColorType::Count; }

    Color color(ColorType type) const { return m_colors[type]; }

    void copy_from(const Palette& other);

    Palette(Vector<uint32_t> data) : m_colors(data.vector()), m_color_data(move(data)) {}
    Palette(UniquePtr<Ext::MappedFile> file) : m_colors((uint32_t*) file->data()), m_raw_file(move(file)) {}

private:
    uint32_t* m_colors { nullptr };
    Vector<uint32_t> m_color_data;
    UniquePtr<Ext::MappedFile> m_raw_file;
};
