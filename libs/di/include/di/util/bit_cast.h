#pragma once

#ifndef DI_NO_USE_STD
#include <bit>
#else
namespace std {
template<typename To, typename From>
constexpr To bit_cast(From const& value) noexcept {
    return __builtin_bit_cast(To, value);
}
}
#endif

namespace di::util {
using std::bit_cast;
}

namespace di {
using util::bit_cast;
}
