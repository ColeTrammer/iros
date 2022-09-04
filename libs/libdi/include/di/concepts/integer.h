#pragma once

#include <di/concepts/one_of.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept Integer = OneOf<meta::RemoveCV<T>, char, short, int, long, long long, unsigned char, unsigned short, unsigned int, unsigned long,
                        unsigned long long>;
}
