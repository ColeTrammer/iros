#pragma once

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
}
