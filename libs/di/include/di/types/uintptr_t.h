#pragma once

namespace di::types {
using uintptr_t = __UINTPTR_TYPE__;
}

namespace di {
using types::uintptr_t;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::uintptr_t;
#endif
