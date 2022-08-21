#pragma once

#include <di/util/concepts/one_of.h>
#include <di/util/meta/remove_cv.h>

namespace di::util::concepts {
template<typename T>
concept Integral = OneOf<meta::RemoveCV<T>, char, short, int, long, long long, unsigned char, unsigned short, unsigned int, unsigned long,
                         unsigned long long, char8_t, char16_t, char32_t>;
}
