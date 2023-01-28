#pragma once

#include <di/concepts/one_of.h>
#include <di/meta/remove_cv.h>
#include <di/types/integers.h>

namespace di::concepts {
template<typename T>
concept Integral = OneOf<meta::RemoveCV<T>, bool, signed char, char, short, int, long, long long, unsigned char,
                         unsigned short, unsigned int, unsigned long, unsigned long long,
#ifdef DI_HAVE_128_BIT_INTEGERS
                         types::i128, types::u128,
#endif
                         char8_t, char16_t, char32_t>;
}
