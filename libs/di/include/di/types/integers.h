#pragma once

#include <di/types/ptrdiff_t.h>
#include <di/types/size_t.h>
#include <di/types/ssize_t.h>
#include <di/types/uintptr_t.h>

namespace di::types {
using u8 = __UINT8_TYPE__;
using u16 = __UINT16_TYPE__;
using u32 = __UINT32_TYPE__;
using u64 = __UINT64_TYPE__;

using i8 = __INT8_TYPE__;
using i16 = __INT16_TYPE__;
using i32 = __INT32_TYPE__;
using i64 = __INT64_TYPE__;

#ifdef __x86_64__
#define DI_HAVE_128_BIT_INTEGERS
using u128 = __uint128_t;
using i128 = __int128_t;
#endif

#ifdef DI_HAVE_128_BIT_INTEGERS
using intmax_t = i128;
using uintmax_t = u128;
#else
using intmax_t = i64;
using uintmax_t = u64;
#endif

using usize = size_t;
using isize = ssize_t;

using uptr = uintptr_t;
using iptr = ptrdiff_t;

using umax = uintmax_t;
using imax = intmax_t;
}

namespace di {
#ifdef DI_HAVE_128_BIT_INTEGERS
using types::i128;
using types::u128;
#endif

using types::i16;
using types::i32;
using types::i64;
using types::i8;
using types::u16;
using types::u32;
using types::u64;
using types::u8;

using types::imax;
using types::iptr;
using types::isize;
using types::umax;
using types::uptr;
using types::usize;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::i16;
using di::i32;
using di::i64;
using di::i8;
using di::imax;
using di::iptr;
using di::isize;
using di::u16;
using di::u32;
using di::u64;
using di::u8;
using di::umax;
using di::uptr;
using di::usize;

#ifdef DI_HAVE_128_BIT_INTEGERS
using di::i128;
using di::u128;
#endif
#endif
