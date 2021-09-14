#pragma once

#include <fcntl.h>
#include <liim/bitset.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

class Font {
public:
    static SharedPtr<Font> default_font();
    static SharedPtr<Font> bold_font();

    virtual ~Font();

    virtual const Bitset<uint8_t>* get_for_character(int c) const = 0;

protected:
    Font();
};
