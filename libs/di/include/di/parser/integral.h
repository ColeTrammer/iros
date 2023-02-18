#pragma once

#include <di/math/intcmp/prelude.h>
#include <di/types/prelude.h>
#include <di/util/compile_time_fail.h>

namespace di {
inline namespace literals {
    inline namespace integral_literals {
#define DI_INTEGER_LITERAL(Type)                                 \
    consteval Type operator""_##Type(unsigned long long value) { \
        if (!math::representable_as<Type>(value)) {              \
            util::compile_time_fail<>();                         \
        }                                                        \
        return Type(value);                                      \
    }

        DI_INTEGER_LITERAL(i8)
        DI_INTEGER_LITERAL(u8)
        DI_INTEGER_LITERAL(i16)
        DI_INTEGER_LITERAL(u16)
        DI_INTEGER_LITERAL(i32)
        DI_INTEGER_LITERAL(u32)
        DI_INTEGER_LITERAL(i64)
        DI_INTEGER_LITERAL(u64)
        DI_INTEGER_LITERAL(isize)
        DI_INTEGER_LITERAL(usize)
        DI_INTEGER_LITERAL(iptr)
        DI_INTEGER_LITERAL(uptr)
        DI_INTEGER_LITERAL(imax)
        DI_INTEGER_LITERAL(umax)

#ifdef DI_HAVE_128_BIT_INTEGERS
        DI_INTEGER_LITERAL(i128)
        DI_INTEGER_LITERAL(u128)
#endif

#undef DI_INTEGER_LITERAL

        consteval Byte operator""_b(unsigned long long value) {
            if (!math::representable_as<u8>(value)) {
                util::compile_time_fail<>();
            }
            return Byte(value);
        }
    }
}
}