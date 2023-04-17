#pragma once

#ifndef DI_NO_USE_STD
#include <cstdint>
#else
namespace std {
using ptrdiff_t = decltype(0z);
}
#endif

namespace di::types {
using ptrdiff_t = std::ptrdiff_t;
}
