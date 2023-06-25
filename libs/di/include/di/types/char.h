#pragma once

namespace di::types {
using c8 = char8_t;
using c16 = char16_t;
using c32 = char32_t;
}

namespace di {
using types::c16;
using types::c32;
using types::c8;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::c16;
using di::c32;
using di::c8;
#endif
