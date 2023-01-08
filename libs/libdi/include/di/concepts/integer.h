#pragma once

#include <di/concepts/one_of.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept Integer = OneOf<meta::RemoveCV<T>, signed char, char, short, int, long, long long, unsigned char, unsigned short, unsigned int,
                        unsigned long, unsigned long long, __int128_t, __uint128_t>;
}
