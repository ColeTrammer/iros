#pragma once

#include <di/execution/algorithm/just.h>
#include <di/util/move.h>
#include <di/vocab/error/error.h>

#if __GNUC__ >= 13
// NOTE: GCC 13 thinks that calling DI_TRY() on a di::Result<int&> produces a dangling reference, because the result
// object gets destroyed before the reference is used. However, di::Result<int&> is essentially a pointer type, so this
// is not a problem. Unfortunately, since this occurs in a macro, this warning cannot be suppressed only inside this
// header file.
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

#define DI_TRY_OR_SEND_ERROR(...)                                                                       \
    __extension__({                                                                                     \
        auto __result = (__VA_ARGS__);                                                                  \
        if (!__result) {                                                                                \
            return ::di::execution::just_error(::di::vocab::Error(::di::util::move(__result).error())); \
        }                                                                                               \
        ::di::util::move(__result).__try_did_succeed();                                                 \
    }).__try_move_out()

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TRY_OR_SEND_ERROR)
#define TRY_OR_SEND_ERROR DI_TRY_OR_SEND_ERROR
#endif
