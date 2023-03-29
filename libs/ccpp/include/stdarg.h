#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/va_list.h>

#define va_start(list, x) __builtin_va_start(list, x)
#define va_end(list)      __builtin_va_end(list)
#define va_arg(list, x)   __builtin_va_arg(list, x)

#ifdef __CCPP_C99
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#endif
