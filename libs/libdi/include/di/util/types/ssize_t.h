#pragma once

#ifdef DI_USE_STD
#include <cstddef>
#else
namespace std {
using ssize_t = decltype(0z);
}
#endif

namespace di::util::types {
using ssize_t = std::ssize_t;
}
