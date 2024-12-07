#pragma once

#include "di/types/ptrdiff_t.h"

namespace di::types {
using ssize_t = ptrdiff_t;
}

namespace di {
using types::ssize_t;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::ssize_t;
#endif
