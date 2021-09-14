#pragma once

#include <graphics/font.h>
#include <liim/forward.h>

namespace PSF {
class Font : public ::Font {
public:
    static SharedPtr<Font> create_blank();
    static SharedPtr<Font> create_with_blank_characters(int num_chars);
    static SharedPtr<Font> try_create_from_path(const String& path);

    explicit Font(const String& path);
    explicit Font(int num_chars);
    virtual ~Font() override;

    virtual const Bitset<uint8_t>* get_for_character(int c) const override;

    bool save_to_file(const String& path) const;

private:
    HashMap<int, Bitset<uint8_t>> m_font_map;
};
}
