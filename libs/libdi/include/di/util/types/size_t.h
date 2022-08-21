#pragma once

#ifdef DI_USE_STD
#include <cstddef>
#else
namespace std {
using size_t = decltype(0zu);
}
#endif

namespace di::util::types {
using size_t = std::size_t;
}
