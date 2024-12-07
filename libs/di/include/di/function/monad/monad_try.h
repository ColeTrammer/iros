#pragma once

#include "di/util/move.h"

#if __GNUC__ >= 13
// NOTE: GCC 13 thinks that calling DI_TRY() on a di::Result<int&> produces a dangling reference, because the result
// object gets destroyed before the reference is used. However, di::Result<int&> is essentially a pointer type, so this
// is not a problem. Unfortunately, since this occurs in a macro, this warning cannot be suppressed only inside this
// header file.
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

#define DI_TRY(...)                                           \
    __extension__({                                           \
        auto __result = (__VA_ARGS__);                        \
        if (!__result) {                                      \
            return di::util::move(__result).__try_did_fail(); \
        }                                                     \
        di::util::move(__result).__try_did_succeed();         \
    }).__try_move_out()

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TRY)
#define TRY DI_TRY
#endif
