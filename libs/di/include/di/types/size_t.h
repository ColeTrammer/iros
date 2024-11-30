#pragma once

#ifndef DI_NO_USE_STD
#include <cstddef>
#else
namespace std {
using size_t = decltype(0ZU);
}
#endif

namespace di::types {
using size_t = std::size_t;
}

namespace di {
using types::size_t;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::size_t;
#endif
