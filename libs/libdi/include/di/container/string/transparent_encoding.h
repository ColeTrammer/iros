#pragma once

#include <di/container/string/encoding.h>

namespace di::container::string {
class TransparentEncoding {
public:
    using CodeUnit = char;
    using CodePoint = char;
    using Iterator = char const*;

private:
    constexpr friend bool tag_invoke(types::Tag<encoding::universal>, InPlaceType<TransparentEncoding>) { return true; }
    constexpr friend bool tag_invoke(types::Tag<encoding::contiguous>, InPlaceType<TransparentEncoding>) {
        return true;
    }
    constexpr friend bool tag_invoke(types::Tag<encoding::null_terminated>, InPlaceType<TransparentEncoding>) {
        return true;
    }
};
}
