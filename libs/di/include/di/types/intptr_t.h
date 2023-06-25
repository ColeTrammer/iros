#pragma once

namespace di::types {
using intptr_t = __INTPTR_TYPE__;
}

namespace di {
using types::intptr_t;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::intptr_t;
#endif
