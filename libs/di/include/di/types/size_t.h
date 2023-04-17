#pragma once

#ifndef DI_NO_USE_STD
#include <cstddef>
#else
namespace std {
using size_t = decltype(0zu);
}
#endif

namespace di::types {
using size_t = std::size_t;
}
