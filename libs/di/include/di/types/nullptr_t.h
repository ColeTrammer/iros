#pragma once

#ifndef DI_NO_USE_STD
#include <cstddef>
#else
namespace std {
using nullptr_t = decltype(nullptr);
}
#endif

namespace di::types {
using nullptr_t = std::nullptr_t;
}

namespace di {
using types::nullptr_t;
}
