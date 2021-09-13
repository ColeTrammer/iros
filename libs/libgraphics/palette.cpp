#include <ext/json.h>
#include <graphics/palette.h>

SharedPtr<Palette> Palette::create_default() {
    Vector<uint32_t> colors(ColorType::Count);

#define __ENUMERATE_COLOR_TYPE(t, l, d) colors.add(Color(d).color());
    ENUMERATE_COLOR_TYPES
#undef __ENUMERATE_COLOR_TYPE

    return make_shared<Palette>(move(colors), "Default Palette");
}

SharedPtr<Palette> Palette::create_from_json(const String& path) {
    auto maybe_object = Ext::Json::parse_file(path);
    if (!maybe_object.has_value()) {
        return nullptr;
    }

    Vector<uint32_t> colors(ColorType::Count);

    auto& object = maybe_object.value();
    auto name = object.get_or<String>("name", path);

    auto maybe_palette = object.get_as<Ext::Json::Object>("palette");
    if (!maybe_palette) {
        return {};
    }

#define __ENUMERATE_COLOR_TYPE(t, l, d)                           \
    {                                                             \
        auto s = maybe_palette->get_as<Ext::Json::String>("" #l); \
        Color color = d;                                          \
        if (s) {                                                  \
            auto maybe_color = Color::parse(s->view());           \
            if (maybe_color.has_value()) {                        \
                color = maybe_color.value();                      \
            }                                                     \
        }                                                         \
        colors.add(color.color());                                \
    }
    ENUMERATE_COLOR_TYPES
#undef __ENUMERATE_COLOR_TYPE

    return make_shared<Palette>(move(colors), move(name));
}

SharedPtr<Palette> Palette::create_from_shared_memory(const String& path, int prot) {
    auto buffer = Ext::try_map_shared_memory(path, prot);
    if (!buffer || buffer->size() != byte_size()) {
        return nullptr;
    }
    return make_shared<Palette>(move(buffer));
}

void Palette::copy_from(const Palette& other) {
    memcpy(this->m_colors, other.m_colors, byte_size());
    m_name = other.m_name;
}
