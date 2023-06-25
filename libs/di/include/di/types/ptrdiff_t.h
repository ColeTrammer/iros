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

namespace di {
using types::ptrdiff_t;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::ptrdiff_t;
#endif
