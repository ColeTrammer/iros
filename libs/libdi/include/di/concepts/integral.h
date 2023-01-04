#pragma once

#include <di/concepts/one_of.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept Integral = OneOf<meta::RemoveCV<T>, bool, signed char, char, short, int, long, long long, unsigned char, unsigned short,
                         unsigned int, unsigned long, unsigned long long, char8_t, char16_t, char32_t>;
}
