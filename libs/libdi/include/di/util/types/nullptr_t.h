#pragma once

#ifdef DI_USE_STD
#include <cstddef>
#else
namespace std {
using nullptr_t = decltype(nullptr);
}
#endif

namespace di::util::types {
using nullptr_t = std::nullptr_t;
}
